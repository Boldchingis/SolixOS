#include "gui/WindowManager.h"

#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QRandomGenerator>
#include <QDebug>

namespace SolixOS {
namespace GUI {

WindowManager::WindowManager(QObject *parent)
    : QObject(parent)
{
}

WindowManager::~WindowManager()
{
    // Clean up windows
    for (auto it = m_windows.begin(); it != m_windows.end(); ++it) {
        if (it->item) {
            it->item->deleteLater();
        }
    }
    m_windows.clear();
}

QQuickItem* WindowManager::createWindow(const QString &title, const QString &qmlSource, QQuickItem *parent)
{
    if (!parent) {
        qWarning() << "Cannot create window: parent is null";
        return nullptr;
    }
    
    QQmlEngine *engine = qmlEngine(parent);
    if (!engine) {
        qWarning() << "Cannot create window: QML engine is null";
        return nullptr;
    }
    
    // Create window component
    QQmlComponent component(engine, QUrl(qmlSource));
    if (component.isError()) {
        qWarning() << "Failed to load window component:" << component.errors();
        return nullptr;
    }
    
    // Create window instance
    QQuickItem *window = qobject_cast<QQuickItem*>(component.create());
    if (!window) {
        qWarning() << "Failed to create window instance";
        return nullptr;
    }
    
    // Set window properties
    window->setParentItem(parent);
    window->setParent(engine);
    window->setProperty("title", title);
    
    // Generate window ID and store window data
    QString windowId = generateWindowId();
    WindowData data;
    data.item = window;
    data.minimized = false;
    data.maximized = false;
    data.normalGeometry = QRect(100, 100, 800, 600);
    m_windows[windowId] = data;
    
    // Set object name for identification
    window->setObjectName(windowId);
    
    // Connect signals
    connect(window, &QQuickItem::visibleChanged, this, [this, windowId](bool visible) {
        if (!visible) {
            emit windowClosed(windowId);
        }
    });
    
    emit windowCreated(windowId, window);
    return window;
}

void WindowManager::minimizeWindow(const QString &windowId)
{
    auto it = m_windows.find(windowId);
    if (it == m_windows.end()) return;
    
    it->minimized = true;
    if (it->item) {
        it->item->setVisible(false);
    }
    emit windowMinimized(windowId);
}

void WindowManager::maximizeWindow(const QString &windowId)
{
    auto it = m_windows.find(windowId);
    if (it == m_windows.end()) return;
    
    if (it->maximized) {
        // Restore
        if (it->item) {
            it->item->setProperty("width", it->normalGeometry.width());
            it->item->setProperty("height", it->normalGeometry.height());
            it->item->setProperty("x", it->normalGeometry.x());
            it->item->setProperty("y", it->normalGeometry.y());
        }
        it->maximized = false;
    } else {
        // Maximize
        if (it->item) {
            it->normalGeometry = QRect(
                it->item->property("x").toInt(),
                it->item->property("y").toInt(),
                it->item->width(),
                it->item->height()
            );
            
            QQuickItem *parent = it->item->parentItem();
            if (parent) {
                it->item->setProperty("x", 0);
                it->item->setProperty("y", 0);
                it->item->setProperty("width", parent->width());
                it->item->setProperty("height", parent->height());
            }
        }
        it->maximized = true;
    }
    
    emit windowMaximized(windowId, it->maximized);
}

void WindowManager::closeWindow(const QString &windowId)
{
    auto it = m_windows.find(windowId);
    if (it == m_windows.end()) return;
    
    if (it->item) {
        it->item->deleteLater();
    }
    
    m_windows.erase(it);
    emit windowClosed(windowId);
}

void WindowManager::focusWindow(const QString &windowId)
{
    auto it = m_windows.find(windowId);
    if (it == m_windows.end() || !it->item) return;
    
    // Raise window
    QQuickItem *window = it->item;
    QQuickItem *parent = window->parentItem();
    if (parent) {
        window->stackAfter(parent->childItems().last());
    }
    
    // Update z-order
    for (auto &pair : m_windows) {
        if (pair.item) {
            pair.item->setProperty("z", pair.item == window ? 1 : 0);
        }
    }
    
    m_activeWindow = windowId;
    emit windowFocused(windowId);
}

void WindowManager::bringToFront(const QString &windowId)
{
    auto it = m_windows.find(windowId);
    if (it == m_windows.end() || !it->item) return;
    
    QQuickItem *window = it->item;
    QQuickItem *parent = window->parentItem();
    if (parent) {
        window->stackAfter(parent->childItems().last());
    }
}

void WindowManager::sendToBack(const QString &windowId)
{
    auto it = m_windows.find(windowId);
    if (it == m_windows.end() || !it->item) return;
    
    QQuickItem *window = it->item;
    QQuickItem *parent = window->parentItem();
    if (parent) {
        window->stackBefore(parent->childItems().first());
    }
}

QString WindowManager::generateWindowId() const
{
    static const QString chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    QString id;
    id.reserve(16);
    
    for (int i = 0; i < 16; ++i) {
        id.append(chars.at(QRandomGenerator::global()->bounded(chars.length())));
    }
    
    return id;
}

} // namespace GUI
} // namespace SolixOS
