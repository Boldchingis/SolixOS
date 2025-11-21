import QtQuick 2.15
import "../theme"

Rectangle {
    id: window
    
    // Properties
    property string title: "Window"
    property bool active: false
    property bool closable: true
    property bool minimizable: true
    property bool resizable: true
    
    // Visuals
    width: 800
    height: 600
    radius: Theme.radiusLarge
    color: Theme.surface
    border {
        width: 1
        color: active ? Theme.primary : Theme.surfaceVariant
    }
    
    // Shadow
    layer.enabled: true
    layer.effect: ShaderEffect {
        property color color: Qt.rgba(0, 0, 0, 0.15)
        
        fragmentShader: "
            varying highp vec2 qt_TexCoord0;
            uniform highp float qt_Opacity;
            uniform lowp vec4 color;
            uniform lowp float radius;
            
            void main() {
                highp float dist = length(2.0 * qt_TexCoord0 - 1.0);
                highp float alpha = smoothstep(0.5, 0.0, dist);
                gl_FragColor = color * alpha * qt_Opacity;
            }
        "
    }
    
    // Title bar
    Rectangle {
        id: titleBar
        width: parent.width
        height: 40
        color: active ? Theme.surfaceVariant : Qt.darker(Theme.surfaceVariant, 1.1)
        radius: parent.radius
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        
        // Title text
        Text {
            text: window.title
            color: Theme.text
            font: Theme.defaultFont
            font.bold: true
            anchors {
                left: parent.left
                leftMargin: 16
                verticalCenter: parent.verticalCenter
            }
        }
        
        // Window controls
        Row {
            anchors {
                right: parent.right
                rightMargin: 8
                verticalCenter: parent.verticalCenter
            }
            spacing: 4
            
            Button {
                text: "—"
                flat: true
                visible: window.minimizable
                onClicked: window.visible = false
            }
            
            Button {
                text: "✕"
                flat: true
                visible: window.closable
                onClicked: window.destroy()
            }
        }
        
        // Drag handler
        MouseArea {
            anchors.fill: parent
            property point clickPos: "0,0"
            
            onPressed: {
                clickPos = Qt.point(mouse.x, mouse.y)
                window.raise()
            }
            
            onPositionChanged: {
                if (pressed) {
                    window.x += mouse.x - clickPos.x
                    window.y += mouse.y - clickPos.y
                }
            }
        }
    }
    
    // Content area
    Item {
        id: contentArea
        anchors {
            top: titleBar.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            margins: 1
            bottomMargin: 0
        }
        clip: true
    }
    
    // Resize handle
    MouseArea {
        id: resizeHandle
        width: 16
        height: 16
        anchors {
            right: parent.right
            bottom: parent.bottom
        }
        cursorShape: Qt.SizeFDiagCursor
        visible: window.resizable
        
        property point clickPos: "0,0"
        
        onPressed: {
            clickPos = Qt.point(mouse.x, mouse.y)
        }
        
        onPositionChanged: {
            if (pressed) {
                window.width = Math.max(window.minimumWidth || 200, window.width + mouse.x - clickPos.x)
                window.height = Math.max(window.minimumHeight || 100, window.height + mouse.y - clickPos.y)
            }
        }
        
        Rectangle {
            anchors.fill: parent
            color: "transparent"
            border {
                width: 2
                color: Theme.surfaceVariant
            }
            
            Canvas {
                anchors.fill: parent
                onPaint: {
                    var ctx = getContext("2d")
                    ctx.strokeStyle = Theme.surfaceVariant
                    ctx.lineWidth = 1
                    
                    // Draw diagonal lines for resize handle
                    for (var i = 0; i < 4; i++) {
                        ctx.beginPath()
                        ctx.moveTo(width, height - i * 4)
                        ctx.lineTo(width - (4 - i) * 4, height)
                        ctx.stroke()
                    }
                }
            }
        }
    }
}
