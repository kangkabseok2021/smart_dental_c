// qml/main.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    visible: true
    width: 900
    height: 660
    minimumWidth: 720
    minimumHeight: 560
    title: "Smart Dental Handpiece"
    color: "#0d1117"

    // ── Status bar ──────────────────────────────────────────────────────────
    Rectangle {
        id: statusBar
        anchors { top: parent.top; left: parent.left; right: parent.right }
        height: 44
        color: "#0a0e14"
        border.color: "#21262d"
        border.width: 1

        RowLayout {
            anchors { fill: parent; leftMargin: 16; rightMargin: 16 }
            spacing: 12

            Rectangle {
                width: 8; height: 8; radius: 4
                color: stateManager.isEmergency ? "#ef4444"
                     : stateManager.isFault     ? "#f59e0b"
                     : stateManager.stateString === "ACTIVE" ? "#4ade80"
                     : "#4b5563"
            }

            Text {
                text: "SMART DENTAL HANDPIECE"
                color: "#94a3b8"
                font { family: "Courier New"; pixelSize: 11; letterSpacing: 2 }
            }

            Item { Layout.fillWidth: true }

            Rectangle {
                width: stateBadgeText.implicitWidth + 16
                height: 22
                radius: 3
                color: stateManager.isEmergency ? "#3b0a0a"
                     : stateManager.isFault     ? "#3b2800"
                     : stateManager.stateString === "ACTIVE" ? "#0f4c2a"
                     : "#1e2738"
                border.color: stateManager.isEmergency ? "#ef4444"
                            : stateManager.isFault     ? "#f59e0b"
                            : stateManager.stateString === "ACTIVE" ? "#4ade80"
                            : "#30363d"
                border.width: 1

                Text {
                    id: stateBadgeText
                    anchors.centerIn: parent
                    text: stateManager.stateString
                    color: stateManager.isEmergency ? "#ef4444"
                         : stateManager.isFault     ? "#f59e0b"
                         : stateManager.stateString === "ACTIVE" ? "#4ade80"
                         : "#94a3b8"
                    font { family: "Courier New"; pixelSize: 10; letterSpacing: 1 }
                }
            }

            Text {
                text: "v" + appVersion
                color: "#4b5563"
                font { family: "Courier New"; pixelSize: 10 }
            }
        }
    }

    // ── Main content ─────────────────────────────────────────────────────────
    Dashboard {
        anchors {
            top: statusBar.bottom
            left: parent.left; right: parent.right; bottom: parent.bottom
            margins: 16
        }
    }

    // ── FAULT overlay ────────────────────────────────────────────────────────
    Rectangle {
        visible: stateManager.isFault
        anchors { top: statusBar.bottom; left: parent.left; right: parent.right }
        height: 52
        color: "#1a1000"
        border.color: "#f59e0b"
        border.width: 1
        z: 10

        RowLayout {
            anchors { fill: parent; leftMargin: 16; rightMargin: 16 }

            Text {
                text: "⚠  FAULT — " + stateManager.faultDetail
                color: "#f59e0b"
                font { pixelSize: 13 }
                Layout.fillWidth: true
            }

            Button {
                text: "ACKNOWLEDGE"
                onClicked: stateManager.acknowledgeFault()
                contentItem: Text {
                    text: parent.text
                    color: "#f59e0b"
                    font { pixelSize: 11; letterSpacing: 1 }
                    horizontalAlignment: Text.AlignHCenter
                }
                background: Rectangle {
                    color: parent.hovered ? "#2a1a00" : "#1a1000"
                    border.color: "#f59e0b"
                    border.width: 1
                    radius: 3
                }
            }
        }
    }

    // ── EMERGENCY STOP overlay ────────────────────────────────────────────────
    Rectangle {
        visible: stateManager.isEmergency
        anchors.fill: parent
        color: "#1a0000"
        opacity: 0.97
        z: 20

        Column {
            anchors.centerIn: parent
            spacing: 20

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "EMERGENCY STOP"
                color: "#ef4444"
                font { family: "Courier New"; pixelSize: 32; bold: true; letterSpacing: 4 }
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: stateManager.faultDetail
                color: "#94a3b8"
                font { pixelSize: 14 }
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Device is locked. Inspect hardware before clearing."
                color: "#6b7280"
                font { pixelSize: 12 }
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "CLEAR EMERGENCY & RESET"
                onClicked: stateManager.clearEmergency()
                contentItem: Text {
                    text: parent.text
                    color: "#ef4444"
                    font { pixelSize: 12; letterSpacing: 1 }
                    horizontalAlignment: Text.AlignHCenter
                }
                background: Rectangle {
                    color: parent.hovered ? "#300000" : "#200000"
                    border.color: "#ef4444"
                    border.width: 1
                    radius: 4
                    implicitWidth: 260
                    implicitHeight: 40
                }
            }
        }
    }
}
