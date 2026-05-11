// qml/Dashboard.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCharts

Item {
    id: root

    // ── Vitals row ────────────────────────────────────────────────────────────
    RowLayout {
        id: vitalsRow
        anchors { top: parent.top; left: parent.left; right: parent.right }
        height: 80
        spacing: 12

        Repeater {
            model: [
                { label: "RPM",     value: telemetryBridge.rpm.toLocaleString(Qt.locale(), 'f', 0),  color: "#4ade80" },
                { label: "TEMP °C", value: telemetryBridge.temperature.toLocaleString(Qt.locale(), 'f', 1), color: telemetryBridge.temperature > 60 ? "#ef4444" : telemetryBridge.temperature > 50 ? "#f59e0b" : "#4ade80" },
                { label: "HOURS",   value: telemetryBridge.usageHours.toLocaleString(Qt.locale(), 'f', 1),  color: "#94a3b8" }
            ]

            Rectangle {
                Layout.fillWidth: true
                height: 80
                color: "#161b22"
                border.color: "#30363d"
                border.width: 1
                radius: 4

                Column {
                    anchors.centerIn: parent
                    spacing: 4

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: modelData.label
                        color: "#4b5563"
                        font { family: "Courier New"; pixelSize: 9; letterSpacing: 1 }
                    }
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: modelData.value
                        color: modelData.color
                        font { family: "Courier New"; pixelSize: 22; bold: true }
                    }
                }
            }
        }
    }

    // ── RPM Chart ─────────────────────────────────────────────────────────────
    ChartView {
        id: rpmChart
        anchors {
            top: vitalsRow.bottom
            left: parent.left; right: parent.right
            topMargin: 12
        }
        height: parent.height * 0.38
        backgroundColor: "#161b22"
        plotAreaColor: "#161b22"
        legend.visible: false
        antialiasing: true
        margins { top: 8; bottom: 8; left: 0; right: 0 }

        ValueAxis {
            id: axisX
            min: 0; max: 30
            labelsColor: "#4b5563"
            gridLineColor: "#21262d"
            labelsFont.pixelSize: 9
            titleText: ""
        }
        ValueAxis {
            id: axisY
            min: 0; max: 45000
            labelsColor: "#4b5563"
            gridLineColor: "#21262d"
            labelsFont.pixelSize: 9
            titleText: ""
        }

        LineSeries {
            id: rpmSeries
            axisX: axisX
            axisY: axisY
            color: "#4ade80"
            width: 1.5
        }

        Connections {
            target: telemetryBridge
            function onRpmPointAdded(x, y) {
                if (rpmSeries.count > 300) rpmSeries.remove(0)
                rpmSeries.append(x, y)
                if (x > axisX.max) {
                    axisX.min = x - 30
                    axisX.max = x
                }
            }
        }
    }

    // ── Controls ──────────────────────────────────────────────────────────────
    RowLayout {
        id: controlsRow
        anchors {
            top: rpmChart.bottom
            left: parent.left; right: parent.right
            topMargin: 12
        }
        height: 72
        spacing: 12

        Repeater {
            model: [
                { label: "POWER",     color: "#4ade80" },
                { label: "INTENSITY", color: "#60a5fa" }
            ]

            Rectangle {
                Layout.fillWidth: true
                height: 72
                color: "#161b22"
                border.color: "#30363d"
                border.width: 1
                radius: 4

                Column {
                    anchors { fill: parent; margins: 12 }
                    spacing: 8

                    Text {
                        text: modelData.label
                        color: "#4b5563"
                        font { family: "Courier New"; pixelSize: 9; letterSpacing: 1 }
                    }

                    Slider {
                        width: parent.width
                        from: 0; to: 100; value: 50
                        enabled: stateManager.stateString === "ACTIVE"
                        opacity: enabled ? 1.0 : 0.35

                        background: Rectangle {
                            x: parent.leftPadding
                            y: parent.topPadding + parent.availableHeight / 2 - height / 2
                            width: parent.availableWidth
                            height: 5
                            radius: 2
                            color: "#0d1117"

                            Rectangle {
                                width: parent.parent.visualPosition * parent.width
                                height: parent.height
                                radius: parent.radius
                                color: modelData.color
                            }
                        }

                        handle: Rectangle {
                            x: parent.leftPadding + parent.visualPosition * (parent.availableWidth - width)
                            y: parent.topPadding + parent.availableHeight / 2 - height / 2
                            width: 14; height: 14; radius: 7
                            color: modelData.color
                            border.color: "#0d1117"
                            border.width: 2
                        }
                    }
                }
            }
        }
    }

    // ── Action buttons ────────────────────────────────────────────────────────
    RowLayout {
        anchors {
            top: controlsRow.bottom
            left: parent.left; right: parent.right
            topMargin: 12
        }
        height: 44
        spacing: 8

        Button {
            visible: stateManager.stateString === "STANDBY"
            Layout.fillWidth: true
            text: "POWER ON"
            onClicked: stateManager.startDevice()
            contentItem: Text {
                text: parent.text; color: "#4ade80"
                font { family: "Courier New"; pixelSize: 11; letterSpacing: 1 }
                horizontalAlignment: Text.AlignHCenter
            }
            background: Rectangle {
                color: parent.hovered ? "#0f4c2a" : "#0a2e1a"
                border.color: "#4ade80"; border.width: 1; radius: 3
            }
        }

        Button {
            visible: stateManager.stateString === "READY"
            Layout.fillWidth: true
            text: "START"
            onClicked: stateManager.startProcedure()
            contentItem: Text {
                text: parent.text; color: "#4ade80"
                font { family: "Courier New"; pixelSize: 11; letterSpacing: 1 }
                horizontalAlignment: Text.AlignHCenter
            }
            background: Rectangle {
                color: parent.hovered ? "#0f4c2a" : "#0a2e1a"
                border.color: "#4ade80"; border.width: 1; radius: 3
            }
        }

        Button {
            visible: stateManager.stateString === "ACTIVE"
            Layout.fillWidth: true
            text: "STOP"
            onClicked: stateManager.stopProcedure()
            contentItem: Text {
                text: parent.text; color: "#94a3b8"
                font { family: "Courier New"; pixelSize: 11; letterSpacing: 1 }
                horizontalAlignment: Text.AlignHCenter
            }
            background: Rectangle {
                color: parent.hovered ? "#1e2738" : "#161b22"
                border.color: "#30363d"; border.width: 1; radius: 3
            }
        }

        Item {
            visible: stateManager.stateString === "CALIBRATING"
            Layout.fillWidth: true
            Text {
                anchors.centerIn: parent
                text: "CALIBRATING..."
                color: "#60a5fa"
                font { family: "Courier New"; pixelSize: 11; letterSpacing: 2 }
            }
        }

        Button {
            Layout.preferredWidth: 160
            text: "E-STOP"
            enabled: stateManager.stateString === "ACTIVE" || stateManager.stateString === "READY"
            opacity: enabled ? 1.0 : 0.3
            onClicked: stateManager.onSensorUpdate(30000, 80.0, 0)
            contentItem: Text {
                text: parent.text; color: "#ef4444"
                font { family: "Courier New"; pixelSize: 11; letterSpacing: 1 }
                horizontalAlignment: Text.AlignHCenter
            }
            background: Rectangle {
                color: parent.hovered && parent.enabled ? "#300000" : "#1a0000"
                border.color: "#ef4444"; border.width: 1; radius: 3
            }
        }
    }
}
