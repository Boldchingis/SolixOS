#include "gui/Compositor.h"
#include "gui/WindowManager.h"

#include <QQuickItem>
#include <QQmlContext>
#include <QSurfaceFormat>
#include <QOpenGLContext>
#include <QDebug>
#include <QTimer>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QOpenGLFunctions>
#include <QThread>
#include <QDateTime>
#include <QSGSimpleRectNode>
#include <QQuickWindow>
#include <QQuickRenderControl>
#include <QQuickRenderTarget>
#include <QQuickGraphicsDevice>
#include <QQuickOpenGLUtils>

#ifdef Q_OS_LINUX
#include <sys/sysinfo.h>
#elif defined(Q_OS_WIN)
#include <windows.h>
#include <psapi.h>
#endif

namespace SolixOS {
namespace GUI {

Compositor* Compositor::s_instance = nullptr;

Compositor::Compositor(QWindow *parent)
    : QQuickView(parent)
    , m_qmlEngine(nullptr)
    , m_renderControl(nullptr)
    , m_metricsTimer(new QTimer(this))
    , m_windowManager(std::make_unique<WindowManager>())
    , m_currentTheme("default")
{
    if (s_instance) {
        qFatal("Only one Compositor instance is allowed!");
    }
    s_instance = this;
    
    // Configure OpenGL context
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    format.setSamples(4); // 4x MSAA
    format.setSwapInterval(1); // Enable VSync by default
    setFormat(format);
    
    // Enable threaded render loop and OpenGL context sharing
    setPersistentOpenGLContext(true);
    setPersistentSceneGraph(true);
    
    // Enable automatic buffer swapping in the render thread
    setClearBeforeRendering(true);
    setColor(Qt::transparent);
    
    // Setup performance monitoring
    m_metricsTimer->setInterval(1000); // Update metrics every second
    connect(m_metricsTimer, &QTimer::timeout, this, &Compositor::updatePerformanceMetrics);
    m_metricsTimer->start();
    
    // Initialize performance metrics
    m_currentMetrics = {};
    m_lastFrameTime = QDateTime::currentMSecsSinceEpoch();
    
    // Initialize components
    m_qmlEngine = engine();
    setupQmlEngine();
    setupConnections();
}

Compositor::~Compositor()
{
    // Clean up windows
    for (auto window : m_windows) {
        if (window) {
            window->deleteLater();
        }
    }
    m_windows.clear();
    
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

void Compositor::initialize()
{
    // Initialize rendering before loading QML
    initializeRendering();
    
    // Set window properties before showing
    setTitle("SolixOS Compositor");
    setMinimumSize(QSize(800, 600));
    
    // Enable window decorations and proper window management
    setFlags(flags() | Qt::Window | Qt::WindowSystemMenuHint | 
             Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    
    // Load the main QML file
    setSource(QUrl("qrc:/gui/Shell.qml"));
    setResizeMode(QQuickView::SizeRootObjectToView);
    
    // Show the window after everything is initialized
    showMaximized();
    
    // Load default theme
    setTheme("default");
    
    // Install event filter for global shortcuts
    installEventFilter(this);
}

QString Compositor::createWindow(const QString &title, const QString &qmlSource, const QVariantMap &properties)
{
    QQuickItem *root = qobject_cast<QQuickItem*>(rootObject());
    if (!root) {
        qWarning() << "Root object not ready!";
        return QString();
    }
    
    QQuickItem *window = createWindowInternal(title, qmlSource, properties, root);
    if (!window) {
        qWarning() << "Failed to create window:" << title;
        return QString();
    }
    
    const QString windowId = window->objectName();
    m_windows[windowId] = window;
    m_windowStack.append(windowId);
    
    // Set initial window properties
    window->setProperty("title", title);
    window->setProperty("visible", true);
    
    // Notify about the new window
    emit windowCreated(windowId);
    emit windowCountChanged(m_windows.size());
    
    // Set as active window
    setActiveWindow(windowId);
    
    return windowId;
}

QQuickItem* Compositor::createWindowInternal(const QString &title, const QString &qmlSource, 
                                           const QVariantMap &properties, QQuickItem *parent)
{
    if (!parent) {
        qWarning() << "Invalid parent for window";
        return nullptr;
    }
    
    QQmlComponent component(m_qmlEngine, QUrl(qmlSource));
    if (component.isError()) {
        qWarning() << "Failed to load window component:" << component.errors();
        return nullptr;
    }
    
    QQuickItem *window = qobject_cast<QQuickItem*>(component.createWithInitialProperties(
        {{ "title", title }}
    ));
    
    if (!window) {
        qWarning() << "Failed to create window instance";
        return nullptr;
    }
    
    // Set additional properties
    QVariantMap props = properties;
    for (auto it = props.constBegin(); it != props.constEnd(); ++it) {
        window->setProperty(it.key().toUtf8().constData(), it.value());
    }
    
    // Set parent and ownership
    window->setParentItem(parent);
    window->setParent(m_qmlEngine);
    
    // Generate a unique ID if not set
    if (window->objectName().isEmpty()) {
        window->setObjectName(QUuid::createUuid().toString());
    }
    
    return window;
}

void Compositor::initializeRendering()
{
    // Set up the render control and offscreen surface
    m_renderControl = new QQuickRenderControl(this);
    
    // Create a QQuickWindow that's associated with our render control
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    
    // Enable multisampling for better visual quality
    QSurfaceFormat format = this->format();
    format.setSamples(4);
    setFormat(format);
    
    // Enable vsync by default
    setVSyncEnabled(true);
}

void Compositor::setupQmlEngine()
{
    if (!m_qmlEngine) {
        qFatal("QML Engine is not initialized!");
        return;
    }
    
    // Register C++ types
    qmlRegisterType<WindowManager>("SolixOS.Compositor", 1, 0, "WindowManager");
    
    // Set context properties
    m_qmlEngine->rootContext()->setContextProperty("compositor", this);
    m_qmlEngine->rootContext()->setContextProperty("windowManager", m_windowManager.get());
    
    // Add import paths
    m_qmlEngine->addImportPath("qrc:/imports");
    m_qmlEngine->addImportPath("qrc:/");
    
    // Set up theme paths
    m_themePaths["default"] = "qrc:/themes/Default";
    m_themePaths["dark"] = "qrc:/themes/Dark";
    m_themePaths["light"] = "qrc:/themes/Light";
    
    // Enable threaded rendering
    m_qmlEngine->setObjectOwnership(this, QQmlEngine::CppOwnership);
    
    // Enable network access for QML
    m_qmlEngine->setNetworkAccessManagerFactory(nullptr);
    
    // Set up QML debugging
#ifdef QT_DEBUG
    m_qmlEngine->setOutputWarningsToStandardError(true);
    m_qmlEngine->setOfflineStoragePath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + 
                                      "/qml_cache");
#endif
    m_qmlEngine->addImportPath("qrc:/");
}

void Compositor::setupConnections()
{
    // Connect window manager signals
    connect(m_windowManager.get(), &WindowManager::windowCreated, 
            this, &Compositor::onWindowCreated);
    connect(m_windowManager.get(), &WindowManager::windowClosed,
            this, &Compositor::onWindowClosed);
            
    // Performance monitoring
    connect(this, &QQuickWindow::frameSwapped, this, [this]() {
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        m_currentMetrics.frameTime = now - m_lastFrameTime;
        m_currentMetrics.fps = qRound(1000.0 / m_currentMetrics.frameTime);
        m_lastFrameTime = now;
        
        // Update memory usage
#ifdef Q_OS_LINUX
        struct sysinfo memInfo;
        if (sysinfo(&memInfo) == 0) {
            m_currentMetrics.memoryUsage = (memInfo.totalram - memInfo.freeram) * memInfo.mem_unit / 1024;
        }
#elif defined(Q_OS_WIN)
        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
            m_currentMetrics.memoryUsage = pmc.WorkingSetSize / 1024;
        }
#endif
    });
    
    // Handle application state changes
    connect(qApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState state) {
        if (state == Qt::ApplicationSuspended) {
            // Release resources when app is suspended
            releaseResources();
        } else if (state == Qt::ApplicationActive) {
            // Restore resources when app becomes active
            resetOpenGLState();
        }
    });
}

void Compositor::onWindowCreated(const QString &windowId, QQuickItem *window)
{
    if (!window) {
        qWarning() << "Invalid window created";
        return;
    }
    
    m_windows[windowId] = window;
    m_windowStack.append(windowId);
    
    // Set up window properties
    window->setProperty("windowId", windowId);
    window->setAcceptedMouseButtons(Qt::AllButtons);
    window->setAcceptHoverEvents(true);
    });
    
    emit windowCountChanged(m_windows.size());
}

void Compositor::onWindowClosed(const QString &windowId)
{
    if (m_windows.contains(windowId)) {
        QQuickItem *window = m_windows.take(windowId);
        m_windowStack.removeAll(windowId);
        m_minimizedWindows.remove(windowId);
        
        if (m_activeWindowId == windowId) {
            m_activeWindowId = m_windowStack.isEmpty() ? QString() : m_windowStack.last();
            if (!m_activeWindowId.isEmpty()) {
                emit windowActivated(m_activeWindowId);
            }
        }
        
        // Schedule the window for deletion
        if (window) {
            window->deleteLater();
        }
        
        emit windowClosed(windowId);
        emit windowCountChanged(m_windows.size());
    }
}

void Compositor::setActiveWindow(const QString &windowId)
{
    if (!m_windows.contains(windowId) || m_activeWindowId == windowId) {
        return;
    }
    
    // Move the window to top of the stack
    m_windowStack.removeAll(windowId);
    m_windowStack.append(windowId);
    
    QString oldActive = m_activeWindowId;
    m_activeWindowId = windowId;
    
    // Update window states
    QQuickItem *window = m_windows[windowId];
    if (window) {
        window->setProperty("active", true);
        window->setZ(1.0);
        window->update();
    }
    
    // Update previously active window
    if (!oldActive.isEmpty() && m_windows.contains(oldActive)) {
        QQuickItem *prevWindow = m_windows[oldActive];
        if (prevWindow) {
            prevWindow->setProperty("active", false);
            prevWindow->setZ(0.0);
            prevWindow->update();
        }
    }
    
    emit windowActivated(windowId);
}

void Compositor::minimizeWindow(const QString &windowId)
{
    if (!m_windows.contains(windowId) || m_minimizedWindows.contains(windowId)) {
        return;
    }
    
    QQuickItem *window = m_windows[windowId];
    if (window) {
        window->setVisible(false);
        m_minimizedWindows.insert(windowId);
        emit windowMinimized(windowId);
    }
}

void Compositor::maximizeWindow(const QString &windowId)
{
    if (!m_windows.contains(windowId)) {
        return;
    }
    
    QQuickItem *window = m_windows[windowId];
    if (window) {
        bool isMaximized = window->property("maximized").toBool();
        window->setProperty("maximized", !isMaximized);
        
        if (!isMaximized) {
            emit windowMaximized(windowId);
        } else {
            emit windowRestored(windowId);
        }
    }
}

void Compositor::restoreWindow(const QString &windowId)
{
    if (!m_windows.contains(windowId)) {
        return;
    }
    
    QQuickItem *window = m_windows[windowId];
    if (window) {
        window->setVisible(true);
        m_minimizedWindows.remove(windowId);
        window->setProperty("maximized", false);
        emit windowRestored(windowId);
        setActiveWindow(windowId);
    }
}

bool Compositor::closeWindow(const QString &windowId)
{
    if (!m_windows.contains(windowId)) {
        return false;
    }
    
    QQuickItem *window = m_windows[windowId];
    if (!window) {
        return false;
    }
    
    // Emit closing signal and check if it was accepted
    QVariant closingResult;
    QMetaObject::invokeMethod(window, "closing", Q_RETURN_ARG(QVariant, closingResult));
    
    if (closingResult.isValid() && !closingResult.toBool()) {
        return false; // Close was cancelled
    }
    
    // Close the window
    m_windows.remove(windowId);
    m_windowStack.removeAll(windowId);
    m_minimizedWindows.remove(windowId);
    
    // Update active window if needed
    if (m_activeWindowId == windowId) {
        m_activeWindowId = m_windowStack.isEmpty() ? QString() : m_windowStack.last();
        if (!m_activeWindowId.isEmpty()) {
            emit windowActivated(m_activeWindowId);
        }
    }
    
    // Schedule the window for deletion
    window->deleteLater();
    
    emit windowClosed(windowId);
    emit windowCountChanged(m_windows.size());
    
    return true;
}

void Compositor::setTheme(const QString &themeName)
{
    if (!m_themePaths.contains(themeName) || m_currentTheme == themeName) {
        return;
    }
    
    m_currentTheme = themeName;
    QString themePath = m_themePaths[themeName];
    
    // Load theme QML file
    QQmlComponent themeComponent(m_qmlEngine, QUrl(themePath + "/Theme.qml"));
    if (themeComponent.isReady()) {
        QObject *theme = themeComponent.create();
        if (theme) {
            // Set the theme as a context property
            m_qmlEngine->rootContext()->setContextProperty("theme", theme);
            emit themeChanged(themeName);
            
            // Force a repaint
            update();
        } else {
            qWarning() << "Failed to create theme:" << themeComponent.errorString();
        }
    } else {
        qWarning() << "Failed to load theme:" << themeComponent.errorString();
    }
}

QString Compositor::currentTheme() const
{
    return m_currentTheme;
}

void Compositor::setVSyncEnabled(bool enabled)
{
    if (m_vsyncEnabled == enabled) {
        return;
    }
    
    m_vsyncEnabled = enabled;
    QSurfaceFormat format = this->format();
    format.setSwapInterval(enabled ? 1 : 0);
    setFormat(format);
    
    // Recreate the OpenGL context
    destroy();
    create();
}

bool Compositor::isVSyncEnabled() const
{
    return m_vsyncEnabled;
}

Compositor::PerformanceMetrics Compositor::getPerformanceMetrics() const
{
    return m_currentMetrics;
}

void Compositor::updatePerformanceMetrics()
{
    // Update window count
    m_currentMetrics.windowCount = m_windows.size();
    
    // Emit updated metrics
    QVariantMap metrics;
    metrics["fps"] = m_currentMetrics.fps;
    metrics["frameTime"] = m_currentMetrics.frameTime;
    metrics["windowCount"] = m_currentMetrics.windowCount;
    metrics["memoryUsage"] = m_currentMetrics.memoryUsage;
    
    emit performanceMetricsUpdated(metrics);
}

bool Compositor::eventFilter(QObject *watched, QEvent *event)
{
    // Handle global shortcuts
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        
        // Alt+Tab window switching
        if (keyEvent->modifiers() & Qt::AltModifier) {
            if (keyEvent->key() == Qt::Key_Tab) {
                if (!m_windowStack.isEmpty()) {
                    int currentIndex = m_windowStack.indexOf(m_activeWindowId);
                    int nextIndex = (currentIndex + 1) % m_windowStack.size();
                    setActiveWindow(m_windowStack[nextIndex]);
                    return true;
                }
            } else if (keyEvent->key() == Qt::Key_F4) {
                // Close active window
                if (!m_activeWindowId.isEmpty()) {
                    closeWindow(m_activeWindowId);
                    return true;
                }
            }
        }
    }
    
    return QQuickView::eventFilter(watched, event);
}

void Compositor::keyPressEvent(QKeyEvent *event)
{
    // Handle global shortcuts that should work even when no window has focus
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->key() == Qt::Key_Q) {
            qApp->quit();
            return;
        } else if (event->key() == Qt::Key_R) {
            // Reload QML (development only)
            engine()->clearComponentCache();
            setSource(QUrl("qrc:/gui/Shell.qml"));
            return;
        }
    }
    
    QQuickView::keyPressEvent(event);
}

void Compositor::resizeEvent(QResizeEvent *event)
{
    QQuickView::resizeEvent(event);
    
    // Update all windows' size hints
    for (QQuickItem *window : qAsConst(m_windows)) {
        if (window) {
            QMetaObject::invokeMethod(window, "updateSizeHints");
        }
    }
}

} // namespace GUI
} // namespace SolixOS
