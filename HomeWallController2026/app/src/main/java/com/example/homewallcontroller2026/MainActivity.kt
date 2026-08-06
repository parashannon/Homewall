package com.example.homewallcontroller2026

import android.Manifest
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.widget.NumberPicker
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.compose.ui.viewinterop.AndroidView
import androidx.core.content.ContextCompat

class MainActivity : ComponentActivity() {

    private lateinit var bleManager: BleManager

    private var connectionStatus by mutableStateOf(
        "Starting Bluetooth..."
    )

    private val bluetoothPermissionLauncher =
        registerForActivityResult(
            ActivityResultContracts.RequestPermission()
        ) { granted ->

            if (granted) {
                bleManager.connect()
            } else {
                connectionStatus =
                    "Bluetooth permission was denied"
            }
        }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        bleManager = BleManager(
            applicationContext,
            object : BleManager.Listener {

                override fun onStatusChanged(status: String) {
                    connectionStatus = status
                }

                override fun onConnected() {
                    connectionStatus =
                        "Connected — HOMEWALL service found"
                }

                override fun onDisconnected() {
                    connectionStatus = "Disconnected"
                }

                override fun onError(message: String) {
                    connectionStatus = message
                }
            }
        )

        setContent {
            MaterialTheme {

                val displayedProblems = remember {
                    createDisplayedProblemNames()
                }

                var selectedProblem by remember {
                    mutableIntStateOf(1)
                }

                Column(
                    modifier = Modifier
                        .fillMaxSize()
                        .padding(20.dp),
                    horizontalAlignment =
                    Alignment.CenterHorizontally,
                    verticalArrangement =
                    Arrangement.Center
                ) {

                    Text(
                        text = connectionStatus,
                        style =
                        MaterialTheme.typography.titleMedium
                    )

                    Spacer(
                        modifier = Modifier.height(20.dp)
                    )

                    Text(
                        text = "Select Problem",
                        style =
                        MaterialTheme.typography.titleLarge
                    )

                    Spacer(
                        modifier = Modifier.height(8.dp)
                    )

                    AndroidView(
                        modifier = Modifier.height(190.dp),

                        factory = { context ->

                            NumberPicker(context).apply {

                                minValue = 1
                                maxValue = 100

                                displayedValues =
                                    displayedProblems

                                value = selectedProblem

                                wrapSelectorWheel = true

                                descendantFocusability =
                                    NumberPicker.FOCUS_BLOCK_DESCENDANTS

                                setOnValueChangedListener {
                                        _,
                                        _,
                                        newValue ->

                                    selectedProblem = newValue
                                }
                            }
                        },

                        update = { numberPicker ->

                            if (
                                numberPicker.value !=
                                selectedProblem
                            ) {
                                numberPicker.value =
                                    selectedProblem
                            }
                        }
                    )

                    Text(
                        text =
                        "Selected: " +
                                displayedProblems[
                                    selectedProblem - 1
                                ],
                        style =
                        MaterialTheme.typography.bodyLarge
                    )

                    Button(
                        modifier =
                        Modifier.padding(top = 14.dp),

                        onClick = {
                            bleManager.sendProblemNumber(
                                selectedProblem
                            )
                        }
                    ) {
                        Text("SET PROBLEM")
                    }

                    Spacer(
                        modifier = Modifier.height(18.dp)
                    )

                    Button(
                        onClick = {
                            tryToConnect()
                        }
                    ) {
                        Text("CONNECT")
                    }

                    Button(
                        modifier =
                        Modifier.padding(top = 8.dp),

                        onClick = {
                            bleManager.disconnect()
                        }
                    ) {
                        Text("DISCONNECT")
                    }
                }
            }
        }

        tryToConnect()
    }

    private fun tryToConnect() {

        if (
            Build.VERSION.SDK_INT >=
            Build.VERSION_CODES.S &&
            ContextCompat.checkSelfPermission(
                this,
                Manifest.permission.BLUETOOTH_CONNECT
            ) != PackageManager.PERMISSION_GRANTED
        ) {
            connectionStatus =
                "Bluetooth permission required"

            bluetoothPermissionLauncher.launch(
                Manifest.permission.BLUETOOTH_CONNECT
            )

        } else {
            bleManager.connect()
        }
    }

    override fun onDestroy() {
        bleManager.close()
        super.onDestroy()
    }

    private fun createDisplayedProblemNames():
            Array<String> {

        val names = arrayOf(
            "Pink is Lava",
            "More Enduro",
            "Abby for Effort",
            "Gentle Giraffe",
            "Beggini Bear",
            "",
            "Good Luck (1-3)",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "Half Orange",
            "",
            "",
            "Blue Square",
            "",
            "Happy Birthday",
            "",
            "",
            "Green",
            "Yellow",
            "",
            "",
            "",
            "Toature",
            "",
            "Half Green",
            "",
            "Green Circle",
            "Take Luck (4-6)",
            "Half Pink",
            "",
            "SS Rhino",
            "",
            "Brown Square",
            "",
            "Pink",
            "",
            "Half Yellow",
            "",
            "Gritty Teeth",
            "",
            "Orange",
            "",
            "Purple",
            "",
            "Fantastic Deer",
            "",
            "Blue",
            "",
            "",
            "Whirly Dirly",
            "",
            "Good Job Koala",
            "",
            "Pink Circle",
            "",
            "Wonderful Hippo",
            "",
            "Eric's Climb",
            "",
            "Fantastic Work Bee",
            "",
            "",
            "Yellow Circle",
            "Jo Dear",
            "",
            "Keep it Up Kitten",
            "",
            "Gooder Luck (7+)",
            "",
            "Half Red",
            "",
            "Erics Other Climb",
            "Could Work",
            "",
            "Green Bandit",
            "",
            "",
            "Red",
            "",
            "",
            "Half Purple",
            "",
            "Half Blue",
            "",
            "",
            "",
            "",
            "",
            "Rando Storage",
            "The Endurance",
            "Rainbow Party"
        )

        return Array(100) { index ->

            val problemNumber = index + 1

            val problemName =
                names.getOrElse(index) { "" }

            if (problemName.isBlank()) {
                problemNumber.toString()
            } else {
                "$problemNumber $problemName"
            }
        }
    }
}