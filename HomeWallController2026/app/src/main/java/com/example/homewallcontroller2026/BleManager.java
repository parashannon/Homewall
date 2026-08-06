package com.example.homewallcontroller2026;

import android.Manifest;
import android.annotation.SuppressLint;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.core.content.ContextCompat;

import java.util.UUID;

public class BleManager {

    private static final String TAG = "HomeWallBLE";

    private static final String DEVICE_ADDRESS =
            BluetoothConstants.DEVICE_ADDRESS;

    public static final UUID SERVICE_UUID =
            BluetoothConstants.SERVICE_UUID;

    private final Context context;
    private final Handler mainHandler;
    private final Listener listener;

    private final BluetoothManager bluetoothManager;

    private BluetoothGatt bluetoothGatt;

    private BluetoothGattCharacteristic problemCharacteristic;

    public interface Listener {

        void onStatusChanged(String status);

        void onConnected();

        void onDisconnected();

        void onError(String message);
    }

    public BleManager(
            @NonNull Context context,
            @NonNull Listener listener
    ) {
        this.context = context.getApplicationContext();
        this.listener = listener;
        this.mainHandler = new Handler(Looper.getMainLooper());

        bluetoothManager =
                (BluetoothManager) this.context.getSystemService(
                        Context.BLUETOOTH_SERVICE
                );
    }

    public boolean hasConnectPermission() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.S) {
            return true;
        }

        return ContextCompat.checkSelfPermission(
                context,
                Manifest.permission.BLUETOOTH_CONNECT
        ) == PackageManager.PERMISSION_GRANTED;
    }

    public boolean isBluetoothAvailable() {
        return bluetoothManager != null
                && bluetoothManager.getAdapter() != null;
    }

    @SuppressLint("MissingPermission")
    public boolean isBluetoothEnabled() {
        if (!hasConnectPermission()) {
            return false;
        }

        BluetoothAdapter adapter = getBluetoothAdapter();

        return adapter != null && adapter.isEnabled();
    }

    @SuppressLint("MissingPermission")
    public void connect() {
        if (!hasConnectPermission()) {
            reportError(
                    "Bluetooth permission has not been granted."
            );
            return;
        }

        BluetoothAdapter adapter = getBluetoothAdapter();

        if (adapter == null) {
            reportError(
                    "Bluetooth is not supported on this tablet."
            );
            return;
        }

        if (!adapter.isEnabled()) {
            reportError("Bluetooth is turned off.");
            return;
        }

        closeGatt();

        BluetoothDevice device;

        try {
            device = adapter.getRemoteDevice(
                    DEVICE_ADDRESS
            );
        } catch (IllegalArgumentException exception) {
            Log.e(
                    TAG,
                    "Invalid Bluetooth address",
                    exception
            );

            reportError(
                    "The HOMEWALL Bluetooth address is invalid."
            );
            return;
        }

        reportStatus("Connecting to HOMEWALL...");

        bluetoothGatt = device.connectGatt(
                context,
                false,
                gattCallback,
                BluetoothDevice.TRANSPORT_LE
        );
    }

    @SuppressLint("MissingPermission")
    public void disconnect() {
        if (!hasConnectPermission()) {
            return;
        }

        if (bluetoothGatt != null) {
            reportStatus("Disconnecting...");
            bluetoothGatt.disconnect();
        }
    }

    public void close() {
        closeGatt();
    }

    private BluetoothAdapter getBluetoothAdapter() {
        if (bluetoothManager == null) {
            return null;
        }

        return bluetoothManager.getAdapter();
    }

    @SuppressLint("MissingPermission")
    private void closeGatt() {
        if (bluetoothGatt == null) {
            return;
        }

        if (hasConnectPermission()) {
            bluetoothGatt.disconnect();
            bluetoothGatt.close();
        }

        bluetoothGatt = null;
        problemCharacteristic = null;
    }

    /**
     * Sends a route/problem number through characteristic 0x0001.
     *
     * This matches the original app's four-byte little-endian
     * integer format.
     */
    @SuppressLint("MissingPermission")
    public void sendProblemNumber(int problemNumber) {
        if (!hasConnectPermission()) {
            reportError(
                    "Bluetooth permission is missing."
            );
            return;
        }

        if (bluetoothGatt == null) {
            reportError(
                    "HOMEWALL is not connected."
            );
            return;
        }

        if (problemCharacteristic == null) {
            reportError(
                    "Problem characteristic is not available."
            );
            return;
        }

        byte[] value = new byte[4];

        value[0] =
                (byte) (problemNumber & 0xFF);

        value[1] =
                (byte) ((problemNumber >> 8) & 0xFF);

        value[2] =
                (byte) ((problemNumber >> 16) & 0xFF);

        value[3] =
                (byte) ((problemNumber >> 24) & 0xFF);

        problemCharacteristic.setWriteType(
                BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
        );

        reportStatus(
                "Sending problem " + problemNumber + "..."
        );

        if (
                Build.VERSION.SDK_INT
                        >= Build.VERSION_CODES.TIRAMISU
        ) {
            int result =
                    bluetoothGatt.writeCharacteristic(
                            problemCharacteristic,
                            value,
                            BluetoothGattCharacteristic
                                    .WRITE_TYPE_DEFAULT
                    );

            if (
                    result
                            != android.bluetooth
                            .BluetoothStatusCodes.SUCCESS
            ) {
                reportError(
                        "Could not start problem write. "
                                + "Result: "
                                + result
                );
            }

        } else {
            problemCharacteristic.setValue(value);

            boolean started =
                    bluetoothGatt.writeCharacteristic(
                            problemCharacteristic
                    );

            if (!started) {
                reportError(
                        "Could not start problem write."
                );
            }
        }
    }

    private final BluetoothGattCallback gattCallback =
            new BluetoothGattCallback() {

                @Override
                public void onConnectionStateChange(
                        @NonNull BluetoothGatt gatt,
                        int status,
                        int newState
                ) {
                    Log.d(
                            TAG,
                            "onConnectionStateChange: status="
                                    + status
                                    + ", newState="
                                    + newState
                    );

                    if (
                            status
                                    != BluetoothGatt.GATT_SUCCESS
                    ) {
                        String message =
                                "Bluetooth connection error: "
                                        + status;

                        Log.e(TAG, message);

                        gatt.close();

                        if (bluetoothGatt == gatt) {
                            bluetoothGatt = null;
                            problemCharacteristic = null;
                        }

                        reportError(message);
                        return;
                    }

                    if (
                            newState
                                    == BluetoothProfile
                                    .STATE_CONNECTED
                    ) {
                        reportStatus(
                                "Connected; discovering services..."
                        );

                        discoverServices(gatt);

                    } else if (
                            newState
                                    == BluetoothProfile
                                    .STATE_DISCONNECTED
                    ) {
                        gatt.close();

                        if (bluetoothGatt == gatt) {
                            bluetoothGatt = null;
                            problemCharacteristic = null;
                        }

                        reportStatus("Disconnected");

                        mainHandler.post(
                                listener::onDisconnected
                        );
                    }
                }

                @Override
                public void onServicesDiscovered(
                        @NonNull BluetoothGatt gatt,
                        int status
                ) {
                    Log.d(
                            TAG,
                            "onServicesDiscovered: status="
                                    + status
                    );

                    if (
                            status
                                    != BluetoothGatt.GATT_SUCCESS
                    ) {
                        reportError(
                                "Service discovery failed: "
                                        + status
                        );
                        return;
                    }

                    BluetoothGattService homeWallService =
                            gatt.getService(
                                    SERVICE_UUID
                            );

                    if (homeWallService == null) {
                        reportError(
                                "Connected, but HOMEWALL service "
                                        + "0x0012 was not found."
                        );
                        return;
                    }

                    Log.d(
                            TAG,
                            "HOMEWALL characteristics:"
                    );

                    for (
                            BluetoothGattCharacteristic characteristic
                            : homeWallService.getCharacteristics()
                    ) {
                        Log.d(
                                TAG,
                                "Characteristic UUID: "
                                        + characteristic.getUuid()
                                        + ", properties: "
                                        + characteristic
                                        .getProperties()
                        );
                    }

                    problemCharacteristic =
                            homeWallService.getCharacteristic(
                                    BluetoothConstants
                                            .PROBLEM_UUID
                            );

                    if (problemCharacteristic == null) {
                        reportError(
                                "Problem characteristic "
                                        + "0x0001 was not found."
                        );
                        return;
                    }

                    reportStatus(
                            "Connected — HOMEWALL service found"
                    );

                    mainHandler.post(
                            listener::onConnected
                    );
                }

                @Override
                public void onCharacteristicWrite(
                        @NonNull BluetoothGatt gatt,
                        @NonNull BluetoothGattCharacteristic characteristic,
                        int status
                ) {
                    Log.d(
                            TAG,
                            "onCharacteristicWrite: UUID="
                                    + characteristic.getUuid()
                                    + ", status="
                                    + status
                    );

                    if (
                            characteristic
                                    .getUuid()
                                    .equals(
                                            BluetoothConstants
                                                    .PROBLEM_UUID
                                    )
                    ) {
                        if (
                                status
                                        == BluetoothGatt.GATT_SUCCESS
                        ) {
                            reportStatus(
                                    "Problem sent successfully"
                            );
                        } else {
                            reportError(
                                    "Problem write failed: "
                                            + status
                            );
                        }
                    }
                }
            };

    @SuppressLint("MissingPermission")
    private void discoverServices(
            BluetoothGatt gatt
    ) {
        if (!hasConnectPermission()) {
            reportError(
                    "Bluetooth permission was lost before "
                            + "service discovery."
            );
            return;
        }

        boolean started =
                gatt.discoverServices();

        if (!started) {
            reportError(
                    "Android could not start "
                            + "service discovery."
            );
        }
    }

    private void reportStatus(
            String status
    ) {
        Log.d(TAG, status);

        mainHandler.post(
                () ->
                        listener.onStatusChanged(
                                status
                        )
        );
    }

    private void reportError(
            String message
    ) {
        Log.e(TAG, message);

        mainHandler.post(
                () -> {
                    listener.onStatusChanged(
                            message
                    );

                    listener.onError(
                            message
                    );
                }
        );
    }
}