#pragma once

#include <QQuickView>
#include <QQmlEngine>
#include <QHash>
#include <QSet>
#include <memory>
#include <atomic>
#include <optional>

// Forward declarations for performance
class QQmlComponent;
class QQuickRenderControl;

namespace SolixOS {
namespace GUI {

class WindowManager;

class Compositor : public QQuickView
{
    Q_OBJECT
    
public:
    explicit Compositor(QWindow *parent = nullptr);
    ~Compositor() override;
    
    void initialize();
    
    // Window management
    Q_INVOKABLE QString createWindow(const QString &title, const QString &qmlSource, const QVariantMap &properties = {});
    Q_INVOKABLE bool closeWindow(const QString &windowId);
    Q_INVOKABLE void setActiveWindow(const QString &windowId);
    Q_INVOKABLE void minimizeWindow(const QString &windowId);
    Q_INVOKABLE void maximizeWindow(const QString &windowId);
    Q_INVOKABLE void restoreWindow(const QString &windowId);
    
    // Window information
    Q_INVOKABLE QVariantMap getWindowInfo(const QString &windowId) const;
    Q_INVOKABLE QVariantList getWindowList() const;
    
    // Theme and appearance
    Q_INVOKABLE void setTheme(const QString &themeName);
    Q_INVOKABLE QString currentTheme() const;
    
    // Performance settings
    void setVSyncEnabled(bool enabled);
    bool isVSyncEnabled() const;
    
    // Get the singleton instance
    static Compositor* instance() { return s_instance; }
    
    // Performance metrics
    struct PerformanceMetrics {
        qint64 frameTime; // in ms
        qint64 renderTime; // in ms
        int fps;
        int windowCount;
        qint64 memoryUsage; // in KB
    };
    
    PerformanceMetrics getPerformanceMetrics() const;
    
signals:
    // Window events
    void windowCountChanged(int count);
    void windowCreated(const QString &windowId);
    void windowClosed(const QString &windowId);
    void windowActivated(const QString &windowId);
    void windowMinimized(const QString &windowId);
    void windowMaximized(const QString &windowId);
    void windowRestored(const QString &windowId);
    
    // Performance events
    void performanceMetricsUpdated(const QVariantMap &metrics);
    
    // System events
    void themeChanged(const QString &themeName);
    void lowMemoryWarning();
    void renderThreadBlocked();
    
private slots:
    void onWindowCreated(const QString &windowId, QQuickItem *window);
    void onWindowClosed(const QString &windowId);
    
private:
    // Initialization
    void setupQmlEngine();
    void setupConnections();
    void initializeRendering();
    
    // Window management helpers
    QQuickItem* createWindowInternal(const QString &title, const QString &qmlSource, 
                                    const QVariantMap &properties, QQuickItem *parent);
    void updateWindowStacking();
    void updatePerformanceMetrics();
    
    // Event handlers
    bool eventFilter(QObject *watched, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    
    // Static members
    static Compositor* s_instance;
    
    // Core components
    std::unique_ptr<WindowManager> m_windowManager;
    QQmlEngine* m_qmlEngine;
    QQuickRenderControl* m_renderControl;
    
    // Window management
    QHash<QString, QQuickItem*> m_windows;
    QSet<QString> m_minimizedWindows;
    QString m_activeWindowId;
    QStringList m_windowStack; // Z-order from bottom to top
    
    // Performance
    std::atomic<bool> m_vsyncEnabled{true};
    QTimer* m_metricsTimer;
    PerformanceMetrics m_currentMetrics;
    qint64 m_lastFrameTime = 0;
    
    // Theme
    QString m_currentTheme;
    QHash<QString, QString> m_themePaths;
};

} // namespace GUI
} // namespace SolixOS
