The main arduino is cdc_acm 1-1.1:1.0
The serial monitor looks for this and returns the ttyamc port

use  nohup python3 http_serial_homewall_monitor.py &
use tail -f all_homewall_serial_output.txt

bluetooth code, is, I think Home_Wall_v8_BluetoothOnly

from sketch directory
arduino-cli compile --fqbn arduino:samd:nano_33_iot /home/pi/HomeWall

Git Hub paramail sdeilers / git--  << doesn't work?

HOMEWALL CONTROL INTERFACE
QUICK COMMAND REFERENCE
===============================================================================

This document summarizes the commands used by the Android app, BLE bridge,
Raspberry Pi, and main HomeWall Arduino.


1. BLUETOOTH LOW ENERGY (BLE) INTERFACE
===============================================================================

HomeWall BLE Service UUID:

    00000012-0000-1000-8000-00805f9b34fb


BLE CHARACTERISTICS
-------------------------------------------------------------------------------

UUID 0001  -  PROBLEM
    App sends:
        32-bit little-endian integer

    Result:
        BLE bridge sends :P<number> to the main Arduino.


UUID 0002  -  HOLD / LED
    App sends:
        32-bit little-endian integer

    Result:
        BLE bridge sends :T<hold> to the main Arduino.


UUID 0003  -  FLIP
    App sends:
        Integer value 1

    Result:
        BLE bridge sends :F to the main Arduino.


UUID 0005  -  RANDOM
    App sends:
        32-bit little-endian integer

    Result:
        BLE bridge sends :R<number> to the main Arduino.


UUID 0006  -  PROBLEM ARRAY
    Purpose:
        Problem-array transfer.


UUID 0007  -  GENERAL STRING COMMAND
    App sends:
        UTF-8 string

    Examples:
        :Q<name>
        :C
        :D
        :W
        :K

    Result:
        Command is passed through to the main Arduino.


NOTE:
    Integer BLE values are sent as four-byte little-endian integers.



2. MAIN ARDUINO SERIAL COMMANDS
===============================================================================

The main Arduino accepts commands from either:

    Serial   = Raspberry Pi / USB
    Serial1  = Bluetooth Arduino / bridge


:P<number>       SET PROBLEM
-------------------------------------------------------------------------------
Example:
    :P42

Result:
    Sets ProblemNumber to 42 and displays the problem.

Special problem numbers:
    1   = special LED drifter mode
    2   = many-moves mode
    99  = special LED drifter mode
    100 = rainbow mode


:F               FLIP PROBLEM
-------------------------------------------------------------------------------
Example:
    :F

Result:
    Toggles flip_problem and redisplays the current problem.


:T<hold>          TOGGLE / MODIFY HOLD
-------------------------------------------------------------------------------
Example:
    :T805

Result:
    Adds, removes, or changes the encoded hold in the current problem,
    then redisplays the problem.


:S               PRINT CURRENT PROBLEM
-------------------------------------------------------------------------------
Example:
    :S

Result:
    Prints the current problem over Serial1.


:X...             LOAD / SET PROBLEM ARRAY
-------------------------------------------------------------------------------
Example:
    :X77-101,-303,806,1010,...

Result:
    Loads a problem array using add_and_set_problem().


:Q<name>          LOOK UP NAMED CLIMB
-------------------------------------------------------------------------------
Example:
    :Qrapid badger

Result:
    Main Arduino prints:

        ilookup:rapid badger

    over USB Serial.

    The Raspberry Pi uses this to search for the named climb and return
    the corresponding problem.


:R<number>        RANDOM PROBLEM COMMAND
-------------------------------------------------------------------------------
Example:
    :R70

Result:
    Sets random_mid and calls randomzie_problem().

    The exact behavior depends on the supplied number. See the RANDOM
    COMMANDS section below.


:V               VERSION
-------------------------------------------------------------------------------
Example:
    :V

Result:
    Prints:

        V <compile date> <compile time>


:C               TOGGLE ARDUINO CLOUD
-------------------------------------------------------------------------------
Example:
    :C

Result:
    Toggles Arduino IoT Cloud updates.

    Serial response is either:

        cloud on

    or:

        cloud off


:D               SHOW HOLD DIFFICULTY
-------------------------------------------------------------------------------
Example:
    :D

Result:
    Sets:

        Alexa_Row = 99

    and calls:

        showDifficulty()

    The wall LEDs are colored according to hold difficulty.


:W               WHITE LIGHT
-------------------------------------------------------------------------------
Example:
    :W

Result:
    Calls:

        setWhite()

    The normal wall LEDs are set to approximately:

        RGB = 120, 120, 120

    Arduino prints:

        White Light


:K               REBOOT HOMEWALL
-------------------------------------------------------------------------------
Example:
    :K

Result:
    Prints:

        Rebooting, bye bye!

    and resets the main Arduino if uptime is greater than 10 seconds.



3. HOLD ENCODING
===============================================================================

Basic hold coordinate:

    hold = row * 100 + column


Examples:

    805
        Row 8, Column 5

    -805
        Row 8, Column 5
        START hold

    10805
        Row 8, Column 5
        END hold


HARD-HOLD ENCODING
-------------------------------------------------------------------------------

A column value greater than 20 indicates the harder use of a hold.

The extra 20 is stripped before locating the physical hold.

Example concept:

    Column 25  -> physical Column 5, hard-use flag enabled



4. RANDOM COMMANDS
===============================================================================

The Android app has two random-problem actions.


RANDOM
-------------------------------------------------------------------------------
App sends a BLE random value equivalent to:

    random_mid = level * 10


GEN RANDOM
-------------------------------------------------------------------------------
App sends a BLE random value equivalent to:

    random_mid = level * 10 - 3


GENERATED CLIMB MODE
-------------------------------------------------------------------------------

If:

    random_mid % 10 == 7

the Arduino generates a new random climb.

Depending on random_mid, the generated climb is stored in one of these
temporary problem slots:

    7
    17
    37
    77


Generated difficulty is calculated as:

    diff_level = random_mid / 10 + 1


During generation, the Arduino prints:

    grw

The Raspberry Pi sees "grw", generates a two-word climb name, and sends
the name back to the Arduino.


The Arduino also prints generation information including:

    Level: <level>  Max Diff: <value>

    iter|hold|nrow|ncol|diff|hrat|minh|info

followed by move-generation details and the final problem information.


The generated name/level is also stored in the status string format:

    L4L<level>:<name>



5. RASPBERRY PI INTERACTION
===============================================================================

GENERATING A NAME
-------------------------------------------------------------------------------

Arduino sends:

    grw

Raspberry Pi:

    Generates a two-word climb name and sends it back over USB Serial.


LOOKING UP A SAVED CLIMB
-------------------------------------------------------------------------------

App / BLE sends:

    :Q<name>

Main Arduino sends to Raspberry Pi:

    ilookup:<name>

Raspberry Pi:

    Searches its climb history/database and returns the associated problem.



6. RECENT-CLIMB HTTP INTERFACE
===============================================================================

Recent climbs are requested from the Raspberry Pi using:

    GET /api/recent-climbs?limit=N


Each climb currently contains:

    name
    level
    timestamp


The Android app displays recent climbs in a vertically scrolling list.

After a generated-random command, the app schedules a recent-climb HTTP
refresh approximately 5 seconds later.

If RANDOM or GEN RANDOM is pressed again before the 5-second delay expires:

    - the previous timer is canceled
    - the 5-second timer restarts



7. CURRENT ANDROID APP CONTROLS
===============================================================================

SET
    Sends the selected problem number through BLE characteristic 0001.
    Equivalent Arduino command:

        :P<number>


LAST
    Reloads the previously selected problem.


FLIP
    Uses BLE characteristic 0003.
    Equivalent Arduino command:

        :F


RANDOM
    Uses BLE characteristic 0005 with:

        level * 10


GEN RANDOM
    Uses BLE characteristic 0005 with:

        level * 10 - 3


DB SET
    Sends:

        :Q<name>


CLEAR
    Clears the selected-hold overlay in the Android app.
    This is primarily a UI action.


CLOUD
    Sends:

        :C


RESET
    Sends:

        :K


SHOW DIFFICULTY
    Sends:

        :D


WHITE
    Sends:

        :W


RECENT CLIMB BUTTON
    Sends:

        :Q<climb name>

    This causes the Raspberry Pi to look up and reload that generated climb.



8. QUICK COMMAND SUMMARY
===============================================================================

    :P<number>    Set problem
    :F            Flip problem
    :T<hold>      Add/remove/change hold
    :S            Print current problem over Serial1
    :X...         Load/set problem array
    :Q<name>      Raspberry Pi climb-name lookup
    :R<number>    Random problem / generated climb command
    :V            Print firmware compile date/time
    :C            Toggle Arduino IoT Cloud
    :D            Display hold difficulty colors
    :W            Set wall LEDs white
    :K            Reboot main Arduino


===============================================================================
END OF QUICK REFERENCE
===============================================================================
