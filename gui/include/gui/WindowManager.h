#pragma once

#include <QObject>
#include <QQuickItem>
#include <QHash>
#include <QRect>

namespace SolixOS {
namespace GUI {

class WindowManager : public QObject
{
    Q_OBJECT
    
public:
    explicit WindowManager(QObject *parent = nullptr);
    ~WindowManager() override;
    
    Q_INVOKABLE QQuickItem* createWindow(const QString &title, const QString &qmlSource, QQuickItem *parent = nullptr);
    
    Q_INVOKABLE void minimizeWindow(const QString &windowId);
    Q_INVOKABLE void maximizeWindow(const QString &windowId);
    Q_INVOKABLE void closeWindow(const QString &windowId);
    Q_INVOKABLE void focusWindow(const QString &windowId);
    
    Q_INVOKABLE void bringToFront(const QString &windowId);
    Q_INVOKABLE void sendToBack(const QString &windowId);
    
signals:
    void windowCreated(const QString &windowId, QQuickItem *window);
    void windowClosed(const QString &windowId);
    void windowMinimized(const QString &windowId);
    void windowMaximized(const QString &windowId, bool maximized);
    void windowFocused(const QString &windowId);
    
private:
    struct WindowData {
        QQuickItem *item;
        bool minimized;
        bool maximized;
        QRect normalGeometry;
    };
    
    QHash<QString, WindowData> m_windows;
    QString m_activeWindow;
    
    QString generateWindowId() const;
};

} // namespace GUI
} // namespace SolixOS
