import QtQuick 2.15
import "../theme"

Rectangle {
    id: root
    
    // Properties
    property string text: "Button"
    property color color: Theme.primary
    property color textColor: Theme.background
    property bool flat: false
    
    // Signals
    signal clicked()
    
    // Sizing
    implicitWidth: content.width + Theme.spacing * 4
    implicitHeight: 36
    radius: Theme.radiusMedium
    
    // Visuals
    color: {
        if (flat) return "transparent"
        return root.enabled ? root.color : Qt.darker(root.color, 1.5)
    }
    
    border {
        width: flat ? 1 : 0
        color: root.enabled ? root.color : Qt.darker(root.color, 1.5)
    }
    
    // Content
    Text {
        id: content
        anchors.centerIn: parent
        text: root.text
        color: flat ? root.color : root.textColor
        font: Theme.defaultFont
        opacity: root.enabled ? 1.0 : 0.6
    }
    
    // Interaction
    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.clicked()
        
        onPressed: {
            if (!root.flat) {
                root.scale = 0.98
            }
        }
        
        onReleased: {
            root.scale = 1.0
        }
        
        onCanceled: {
            root.scale = 1.0
        }
    }
    
    // States
    states: [
        State {
            when: mouseArea.containsMouse && !mouseArea.pressed
            PropertyChanges {
                target: root
                scale: 1.02
                opacity: 0.9
            }
        },
        State {
            when: !mouseArea.containsMouse && !mouseArea.pressed
            PropertyChanges {
                target: root
                scale: 1.0
                opacity: 1.0
            }
        }
    ]
    
    // Transitions
    Behavior on scale { NumberAnimation { duration: Theme.animationFast } }
    Behavior on opacity { NumberAnimation { duration: Theme.animationFast } }
}
