pragma Singleton
import QtQuick 2.15
import QtQuick.Window 2.15

QtObject {
    // ===== Color System =====
    // Base Colors
    readonly property color background: "#1e1e2e"
    readonly property color surface: "#313244"
    readonly property color surfaceVariant: "#45475a"
    readonly property color surfaceBright: "#585b70"
    
    // Text Colors
    readonly property color text: "#cdd6f4"
    readonly property color textSecondary: "#a6adc8"
    readonly property color textTertiary: "#7f849c"
    readonly property color textOnPrimary: "#11111b"
    readonly property color textOnAccent: "#11111b"
    
    // Accent Colors
    readonly property color primary: "#89b4fa"
    readonly property color primaryVariant: "#74c7ec"
    readonly property color secondary: "#f5c2e7"
    readonly property color accent: "#f5e0dc"
    
    // Semantic Colors
    readonly property color success: "#a6e3a1"
    readonly property color warning: "#f9e2af"
    readonly property color error: "#f38ba8"
    readonly property color info: "#89dceb"
    
    // State Colors
    readonly property color hover: Qt.rgba(1, 1, 1, 0.1)
    readonly property color pressed: Qt.rgba(1, 1, 1, 0.2)
    readonly property color focus: Qt.rgba(137, 180, 250, 0.3)
    readonly property color selection: Qt.rgba(166, 173, 200, 0.2)
    
    // ===== Typography =====
    // Font Families
    readonly property string fontFamily: "Inter, -apple-system, Segoe UI, Noto Sans, sans-serif"
    readonly property string monoFontFamily: "JetBrains Mono, Fira Code, monospace"
    
    // Font Sizes (in pixels)
    readonly property int fontSizeXs: 10
    readonly property int fontSizeSm: 12
    readonly property int fontSizeMd: 14
    readonly property int fontSizeLg: 16
    readonly property int fontSizeXl: 18
    readonly property int fontSize2xl: 24
    readonly property int fontSize3xl: 32
    readonly property int fontSize4xl: 48
    
    // Font Weights
    readonly property int fontWeightLight: Font.Light
    readonly property int fontWeightNormal: Font.Normal
    readonly property int fontWeightMedium: Font.Medium
    readonly property int fontWeightBold: Font.Bold
    
    // Line Heights
    readonly property real lineHeightTight: 1.2
    readonly property real lineHeightNormal: 1.5
    readonly property real lineHeightLoose: 1.8
    
    // Typography Presets
    readonly property font displayLarge: Qt.font({
        family: fontFamily,
        pixelSize: fontSize4xl,
        weight: fontWeightBold,
        letterSpacing: -0.5
    })
    
    readonly property font displayMedium: Qt.font({
        family: fontFamily,
        pixelSize: fontSize3xl,
        weight: fontWeightBold,
        letterSpacing: -0.5
    })
    
    readonly property font headlineLarge: Qt.font({
        family: fontFamily,
        pixelSize: fontSize2xl,
        weight: fontWeightBold
    })
    
    readonly property font titleLarge: Qt.font({
        family: fontFamily,
        pixelSize: fontSizeXl,
        weight: fontWeightBold
    })
    
    readonly property font bodyLarge: Qt.font({
        family: fontFamily,
        pixelSize: fontSizeLg,
        weight: fontWeightNormal,
        lineHeight: lineHeightNormal
    })
    
    readonly property font bodyMedium: Qt.font({
        family: fontFamily,
        pixelSize: fontSizeMd,
        weight: fontWeightNormal,
        lineHeight: lineHeightNormal
    })
    
    readonly property font labelLarge: Qt.font({
        family: fontFamily,
        pixelSize: fontSizeMd,
        weight: fontWeightMedium,
        letterSpacing: 0.1
    })
    
    // ===== Spacing System =====
    // Base unit is 4px
    readonly property int space0: 0
    readonly property int space1: 4
    readonly property int space2: 8
    readonly property int space3: 12
    readonly property int space4: 16
    readonly property int space5: 20
    readonly property int space6: 24
    readonly property int space8: 32
    readonly property int space10: 40
    readonly property int space12: 48
    readonly property int space16: 64
    
    // Aliases for common spacing
    readonly property int spacing: space2  // 8px
    readonly property int padding: space4  // 16px
    
    // ===== Border Radius =====
    readonly property int radiusXs: 2
    readonly property int radiusSm: 4
    readonly property int radiusMd: 8
    readonly property int radiusLg: 12
    readonly property int radiusXl: 16
    readonly property int radius2xl: 24
    readonly property int radiusFull: 9999
    
    // ===== Elevation & Shadows =====
    function elevation(dp) {
        if (dp === 0) return "transparent"
        
        const alpha = Math.min(0.3, dp * 0.1)
        const y = Math.floor(dp / 2)
        const blur = dp * 1.5
        
        return `0 ${y}px ${blur}px 0 rgba(0, 0, 0, ${alpha})`
    }
    
    // Predefined shadow levels
    readonly property string shadowSm: elevation(1)
    readonly property string shadowMd: elevation(2)
    readonly property string shadowLg: elevation(4)
    readonly property string shadowXl: elevation(8)
    
    // ===== Animation & Transitions =====
    readonly property int durationFast: 100
    readonly property int durationNormal: 200
    readonly property int durationSlow: 300
    
    // Standard easing curves
    readonly property var standardEasing: Easing.OutCubic
    readonly property var deceleratedEasing: Easing.OutQuad
    readonly property var acceleratedEasing: Easing.InQuad
    
    // Standard transition for interactive elements
    function standardTransition(properties) {
        return {
            NumberAnimation {
                properties: properties || "x,y,width,height,opacity,scale"
                duration: Theme.durationNormal
                easing: Theme.standardEasing
            }
        }
    }
    
    // ===== Window & UI Elements =====
    readonly property int titleBarHeight: 32
    readonly property int windowBorderWidth: 1
    readonly property color windowBorder: Qt.rgba(0, 0, 0, 0.1)
    readonly property int windowRadius: radiusLg
    
    // ===== Icons =====
    readonly property int iconSizeXs: 12
    readonly property int iconSizeSm: 16
    readonly property int iconSizeMd: 24
    readonly property int iconSizeLg: 32
    
    // ===== Responsive Breakpoints =====
    function isMobile(width) {
        return width < 640
    }
    
    function isTablet(width) {
        return width >= 640 && width < 1024
    }
    
    function isDesktop(width) {
        return width >= 1024
    }
    
    // Current window width helper
    function windowWidth() {
        return typeof applicationWindow !== 'undefined' ? applicationWindow.width : Screen.width
    }
    
    // ===== Helper Functions =====
    // Opacity utilities
    function opacity(color, alpha) {
        return Qt.rgba(color.r, color.g, color.b, alpha)
    }
    
    // Color manipulation
    function lighten(color, factor) {
        return Qt.lighter(color, 1.0 + factor)
    }
    
    function darken(color, factor) {
        return Qt.darker(color, 1.0 + factor)
    }
    
    // Pixel density aware sizing
    function dp(pixels) {
        return pixels * (Screen.devicePixelRatio || 1)
    }
    
    // ===== Component Styles =====
    // Button Styles
    property Component buttonStyle: Component {
        id: buttonStyle
        
        property color backgroundColor: "transparent"
        property color hoverColor: Theme.hover
        property color pressedColor: Theme.pressed
        property color textColor: Theme.text
        property color borderColor: "transparent"
        property int radius: Theme.radiusMd
        property int padding: Theme.space2
        
        Rectangle {
            id: buttonBackground
            color: control.down ? pressedColor : 
                                 (control.hovered ? hoverColor : backgroundColor)
            border.color: borderColor
            radius: control.radius || buttonStyle.radius
            
            Behavior on color {
                ColorAnimation { duration: Theme.durationFast }
            }
        }
    }
    
    // Text Input Style
    property Component textInputStyle: Component {
        Rectangle {
            color: Theme.surface
            border.color: control.activeFocus ? Theme.primary : Theme.surfaceVariant
            border.width: 1
            radius: Theme.radiusSm
            
            Behavior on border.color {
                ColorAnimation { duration: Theme.durationFast }
            }
        }
    }
}
