package com.example.homewallcontroller2026

import android.Manifest
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.widget.NumberPicker
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.KeyboardActions
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Card
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateListOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalFocusManager
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.viewinterop.AndroidView
import androidx.core.content.ContextCompat

class MainActivity : ComponentActivity() {

    private lateinit var bleManager: BleManager
    private lateinit var homeWallHttpClient: HomeWallHttpClient

    private var connectionStatus by mutableStateOf(
        "Starting Bluetooth..."
    )

    private var recentClimbs by mutableStateOf<List<RecentClimb>>(
        emptyList()
    )

    private var recentClimbStatus by mutableStateOf(
        "Recent climbs not loaded"
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

        homeWallHttpClient = HomeWallHttpClient()

        setContent {
            MaterialTheme {

                val context = LocalContext.current
                val focusManager = LocalFocusManager.current

                val displayedProblems = remember {
                    createDisplayedProblemNames()
                }

                var selectedProblem by remember {
                    mutableIntStateOf(1)
                }

                var lastProblem by remember {
                    mutableIntStateOf(1)
                }

                var randomLevel by remember {
                    mutableIntStateOf(3)
                }

                var databaseName by remember {
                    mutableStateOf("")
                }

                val selectedHolds = remember {
                    mutableStateListOf<Int>()
                }

                val imageResource = remember {
                    val oldImage =
                        context.resources.getIdentifier(
                            "home_wall_cropped_small_update",
                            "drawable",
                            context.packageName
                        )

                    if (oldImage != 0) {
                        oldImage
                    } else {
                        context.resources.getIdentifier(
                            "homewall",
                            "drawable",
                            context.packageName
                        )
                    }
                }

                LaunchedEffect(Unit) {
                    loadRecentClimbs()
                }

                Column(
                    modifier = Modifier
                        .fillMaxSize()
                        .padding(6.dp)
                ) {

                    Text(
                        modifier = Modifier.fillMaxWidth(),
                        text = connectionStatus,
                        fontSize = 11.sp,
                        maxLines = 1,
                        textAlign = TextAlign.Center
                    )

                    Spacer(Modifier.height(3.dp))

                    /*
                     * Top controls.
                     */
                    Row(
                        modifier = Modifier
                            .fillMaxWidth()
                            .height(120.dp),
                        horizontalArrangement =
                        Arrangement.spacedBy(4.dp)
                    ) {

                        AndroidView(
                            modifier = Modifier
                                .weight(1.25f)
                                .fillMaxHeight(),

                            factory = { pickerContext ->
                                NumberPicker(pickerContext).apply {
                                    minValue = 1
                                    maxValue = 100
                                    displayedValues =
                                        displayedProblems
                                    value = selectedProblem
                                    wrapSelectorWheel = true

                                    descendantFocusability =
                                        NumberPicker
                                            .FOCUS_BLOCK_DESCENDANTS

                                    setOnValueChangedListener {
                                            _,
                                            _,
                                            newValue ->

                                        selectedProblem =
                                            newValue
                                    }
                                }
                            },

                            update = { picker ->
                                if (
                                    picker.value
                                    != selectedProblem
                                ) {
                                    picker.value =
                                        selectedProblem
                                }
                            }
                        )

                        Column(
                            modifier = Modifier
                                .weight(0.72f)
                                .fillMaxHeight(),
                            verticalArrangement =
                            Arrangement.SpaceEvenly
                        ) {

                            CompactButton(
                                text = "SET",
                                onClick = {
                                    lastProblem =
                                        selectedProblem

                                    bleManager
                                        .sendProblemNumber(
                                            selectedProblem
                                        )
                                }
                            )

                            CompactButton(
                                text = "LAST",
                                onClick = {
                                    selectedProblem =
                                        lastProblem

                                    bleManager
                                        .sendProblemNumber(
                                            lastProblem
                                        )
                                }
                            )

                            CompactButton(
                                text = "FLIP",
                                onClick = {
                                    bleManager.sendFlip()
                                }
                            )
                        }

                        AndroidView(
                            modifier = Modifier
                                .weight(0.42f)
                                .fillMaxHeight(),

                            factory = { pickerContext ->
                                NumberPicker(pickerContext).apply {
                                    minValue = 1
                                    maxValue = 10
                                    displayedValues =
                                        arrayOf(
                                            "10", "20", "30",
                                            "40", "50", "60",
                                            "70", "80", "90",
                                            "100"
                                        )
                                    value = randomLevel
                                    wrapSelectorWheel = true

                                    descendantFocusability =
                                        NumberPicker
                                            .FOCUS_BLOCK_DESCENDANTS

                                    setOnValueChangedListener {
                                            _,
                                            _,
                                            newValue ->

                                        randomLevel =
                                            newValue
                                    }
                                }
                            },

                            update = { picker ->
                                if (
                                    picker.value
                                    != randomLevel
                                ) {
                                    picker.value =
                                        randomLevel
                                }
                            }
                        )

                        Column(
                            modifier = Modifier
                                .weight(0.95f)
                                .fillMaxHeight(),
                            verticalArrangement =
                            Arrangement.SpaceEvenly
                        ) {

                            CompactButton(
                                text = "RANDOM",
                                onClick = {
                                    bleManager
                                        .sendRandomValue(
                                            randomLevel * 10
                                        )
                                }
                            )

                            CompactButton(
                                text = "GEN RANDOM",
                                onClick = {
                                    bleManager
                                        .sendRandomValue(
                                            randomLevel * 10 - 3
                                        )
                                }
                            )

                            Row(
                                horizontalArrangement =
                                Arrangement.spacedBy(3.dp)
                            ) {
                                CompactButton(
                                    modifier =
                                    Modifier.weight(1f),
                                    text = "CONN",
                                    onClick = {
                                        tryToConnect()
                                    }
                                )

                                CompactButton(
                                    modifier =
                                    Modifier.weight(1f),
                                    text = "DISC",
                                    onClick = {
                                        bleManager.disconnect()
                                    }
                                )
                            }
                        }
                    }

                    Spacer(Modifier.height(3.dp))

                    /*
                     * Database text command and utility controls.
                     */
                    Row(
                        modifier = Modifier
                            .fillMaxWidth()
                            .height(48.dp),
                        horizontalArrangement =
                        Arrangement.spacedBy(4.dp),
                        verticalAlignment =
                        Alignment.CenterVertically
                    ) {

                        OutlinedTextField(
                            modifier = Modifier.weight(1.6f),
                            value = databaseName,
                            onValueChange = {
                                databaseName = it
                            },
                            label = {
                                Text(
                                    "Database climb",
                                    fontSize = 9.sp
                                )
                            },
                            singleLine = true,
                            textStyle =
                            MaterialTheme.typography
                                .bodySmall,
                            keyboardOptions =
                            KeyboardOptions(
                                imeAction =
                                ImeAction.Done
                            ),
                            keyboardActions =
                            KeyboardActions(
                                onDone = {
                                    focusManager.clearFocus()
                                }
                            )
                        )

                        CompactButton(
                            modifier = Modifier.weight(0.7f),
                            text = "DB SET",
                            onClick = {
                                val name =
                                    databaseName.trim()

                                if (name.isNotEmpty()) {
                                    bleManager.sendString(
                                        ":Q$name"
                                    )
                                }
                            }
                        )

                        CompactButton(
                            modifier = Modifier.weight(0.65f),
                            text = "CLEAR",
                            onClick = {
                                selectedHolds.clear()
                            }
                        )

                        CompactButton(
                            modifier = Modifier.weight(0.65f),
                            text = "CLOUD",
                            onClick = {
                                bleManager.sendString(":C")
                            }
                        )

                        CompactButton(
                            modifier = Modifier.weight(0.65f),
                            text = "RESET",
                            onClick = {
                                bleManager.sendString(":K")
                            }
                        )
                    }

                    Spacer(Modifier.height(3.dp))

                    /*
                     * Middle wall image and 16 x 11 clickable grid.
                     */
                    Card(
                        modifier = Modifier
                            .fillMaxWidth()
                            .weight(0.61f)
                    ) {
                        Box(
                            modifier = Modifier.fillMaxSize(),
                            contentAlignment = Alignment.Center
                        ) {
                            /*
                             * The original wall image was laid out at
                             * 571 x 880 dp. The image and the clickable
                             * grid must occupy this exact same aspect ratio.
                             * Otherwise ContentScale.Fit letterboxes the image
                             * while the grid stretches across the whole card.
                             */
                            Box(
                                modifier = Modifier
                                    .fillMaxHeight()
                                    .aspectRatio(
                                        ratio = 571f / 880f,
                                        matchHeightConstraintsFirst = true
                                    )
                            ) {
                                if (imageResource != 0) {
                                    Image(
                                        modifier = Modifier.fillMaxSize(),
                                        painter = painterResource(
                                            imageResource
                                        ),
                                        contentDescription = "HomeWall",
                                        contentScale = ContentScale.FillBounds
                                    )
                                } else {
                                    Text(
                                        modifier = Modifier
                                            .align(Alignment.Center)
                                            .padding(12.dp),
                                        text =
                                        "Add home_wall_cropped_small_update.png\n" +
                                                "or homewall.png to res/drawable",
                                        textAlign = TextAlign.Center
                                    )
                                }

                                Column(
                                    modifier = Modifier.fillMaxSize()
                                ) {
                                    for (displayRow in 0 until 16) {
                                        Row(
                                            modifier = Modifier
                                                .fillMaxWidth()
                                                .weight(1f)
                                        ) {
                                            for (column in 1..11) {
                                                // Physical row 16 is at the top;
                                                // physical row 1 is at the bottom.
                                                val row = 16 - displayRow

                                                // Explicit formatting gives:
                                                // 101, 102 ... 111;
                                                // 201, 202 ... 211; etc.
                                                val holdLabel =
                                                    "%d%02d".format(
                                                        row,
                                                        column
                                                    )

                                                val holdValue =
                                                    holdLabel.toInt()

                                                val isSelected =
                                                    selectedHolds.contains(
                                                        holdValue
                                                    )

                                                Box(
                                                    modifier = Modifier
                                                        .weight(1f)
                                                        .fillMaxHeight()
                                                        .background(
                                                            if (isSelected) {
                                                                Color(0x5533FF33)
                                                            } else {
                                                                Color(0x220000FF)
                                                            }
                                                        )
                                                        .clickable {
                                                            if (isSelected) {
                                                                selectedHolds.remove(
                                                                    holdValue
                                                                )
                                                            } else {
                                                                selectedHolds.add(
                                                                    holdValue
                                                                )
                                                            }

                                                            bleManager.sendLedValue(
                                                                holdValue
                                                            )
                                                        },
                                                    contentAlignment =
                                                    Alignment.Center
                                                ) {
                                                    Text(
                                                        text = holdLabel,
                                                        color = Color.White,
                                                        fontSize = 7.sp,
                                                        maxLines = 1
                                                    )
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Spacer(Modifier.height(3.dp))

                    /*
                     * Bottom recent-climb HTTP panel.
                     */
                    Card(
                        modifier = Modifier
                            .fillMaxWidth()
                            .weight(0.20f)
                    ) {
                        Column(
                            modifier = Modifier
                                .fillMaxSize()
                                .padding(4.dp)
                        ) {

                            Row(
                                modifier =
                                Modifier.fillMaxWidth(),
                                verticalAlignment =
                                Alignment.CenterVertically
                            ) {

                                Text(
                                    modifier =
                                    Modifier.weight(1f),
                                    text =
                                    "Recent climbs — " +
                                            recentClimbStatus,
                                    fontSize = 10.sp,
                                    maxLines = 1
                                )

                                CompactButton(
                                    modifier =
                                    Modifier.weight(0.32f),
                                    text = "REFRESH",
                                    onClick = {
                                        loadRecentClimbs()
                                    }
                                )
                            }

                            Spacer(Modifier.height(2.dp))

                            LazyRow(
                                modifier =
                                Modifier.fillMaxSize(),
                                horizontalArrangement =
                                Arrangement.spacedBy(4.dp)
                            ) {
                                items(recentClimbs) { climb ->

                                    Button(
                                        modifier =
                                        Modifier.fillMaxHeight(),
                                        shape =
                                        RoundedCornerShape(
                                            6.dp
                                        ),
                                        contentPadding =
                                        ButtonDefaults
                                            .ContentPadding,
                                        onClick = {
                                            bleManager.sendString(
                                                ":Q${climb.name}"
                                            )

                                            recentClimbStatus =
                                                "DB set: " +
                                                        "L${climb.level} " +
                                                        climb.name
                                        }
                                    ) {
                                        Column(
                                            horizontalAlignment =
                                            Alignment
                                                .CenterHorizontally
                                        ) {
                                            Text(
                                                text =
                                                "L${climb.level}",
                                                fontSize = 10.sp
                                            )
                                            Text(
                                                text = climb.name,
                                                fontSize = 9.sp,
                                                maxLines = 2,
                                                textAlign =
                                                TextAlign.Center
                                            )
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        tryToConnect()
    }

    private fun loadRecentClimbs() {

        recentClimbStatus =
            "Loading from Pi..."

        homeWallHttpClient.loadRecentClimbs(
            object : HomeWallHttpClient.Listener {

                override fun onClimbsLoaded(
                    climbs: List<RecentClimb>
                ) {
                    recentClimbs = climbs

                    recentClimbStatus =
                        if (climbs.isEmpty()) {
                            "No climbs found"
                        } else {
                            "${climbs.size} loaded"
                        }
                }

                override fun onError(message: String) {
                    recentClimbStatus =
                        "Pi error: $message"
                }
            }
        )
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

        if (::bleManager.isInitialized) {
            bleManager.close()
        }

        if (::homeWallHttpClient.isInitialized) {
            homeWallHttpClient.shutdown()
        }

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
            val number = index + 1
            val name = names.getOrElse(index) { "" }

            if (name.isBlank()) {
                number.toString()
            } else {
                "$number $name"
            }
        }
    }
}

@androidx.compose.runtime.Composable
private fun CompactButton(
    modifier: Modifier = Modifier,
    text: String,
    onClick: () -> Unit
) {
    Button(
        modifier = modifier
            .fillMaxWidth()
            .height(34.dp),
        contentPadding =
        ButtonDefaults.ContentPadding,
        onClick = onClick
    ) {
        Text(
            text = text,
            fontSize = 9.sp,
            maxLines = 1
        )
    }
}