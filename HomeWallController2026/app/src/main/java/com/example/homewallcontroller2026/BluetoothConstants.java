package com.example.homewallcontroller2026;

import java.util.UUID;

public final class BluetoothConstants {

    private BluetoothConstants() {
        // Prevent this constants-only class from being instantiated.
    }

    public static final String DEVICE_ADDRESS =
            "30:C6:F7:02:19:62";

    public static final UUID SERVICE_UUID =
            UUID.fromString(
                    "00000012-0000-1000-8000-00805f9b34fb"
            );

    public static final UUID PROBLEM_UUID =
            UUID.fromString(
                    "00000001-0000-1000-8000-00805f9b34fb"
            );

    public static final UUID LED_UUID =
            UUID.fromString(
                    "00000002-0000-1000-8000-00805f9b34fb"
            );

    public static final UUID FLIP_UUID =
            UUID.fromString(
                    "00000003-0000-1000-8000-00805f9b34fb"
            );

    public static final UUID RANDOM_UUID =
            UUID.fromString(
                    "00000005-0000-1000-8000-00805f9b34fb"
            );

    public static final UUID ARRAY_UUID =
            UUID.fromString(
                    "00000006-0000-1000-8000-00805f9b34fb"
            );

    public static final UUID STRING_UUID =
            UUID.fromString(
                    "00000007-0000-1000-8000-00805f9b34fb"
            );

    /*
     * Proposed new Arduino characteristic:
     * READ + NOTIFY, containing the current climb name as UTF-8.
     */
    public static final UUID CLIMB_NAME_UUID =
            UUID.fromString(
                    "00000008-0000-1000-8000-00805f9b34fb"
            );

    public static final UUID CCCD_UUID =
            UUID.fromString(
                    "00002902-0000-1000-8000-00805f9b34fb"
            );
}