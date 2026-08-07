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

import java.nio.charset.StandardCharsets;

public class BleManager {

    private static final String TAG = "HomeWallBLE";

    private static final int GATT_ERROR_133 = 133;
    private static final int GATT_CONN_TIMEOUT = 8;

    private static final int MAX_CONNECTION_RETRIES = 2;
    private static final long RECONNECT_DELAY_MS = 6000L;
    private static final long OLD_GATT_CLOSE_DELAY_MS = 3000L;

    private final Context context;
    private final Handler mainHandler;
    private final Listener listener;
    private final BluetoothManager bluetoothManager;

    private BluetoothGatt bluetoothGatt;

    private BluetoothGattCharacteristic problemCharacteristic;
    private BluetoothGattCharacteristic ledCharacteristic;
    private BluetoothGattCharacteristic flipCharacteristic;
    private BluetoothGattCharacteristic randomCharacteristic;
    private BluetoothGattCharacteristic arrayCharacteristic;
    private BluetoothGattCharacteristic stringCharacteristic;

    private boolean isConnected = false;
    private boolean connectionInProgress = false;
    private boolean userRequestedDisconnect = false;
    private int connectionRetryCount = 0;

    private final Runnable reconnectRunnable = new Runnable() {
        @Override
        public void run() {
            connectInternal(true);
        }
    };

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

    public boolean isConnected() {
        return isConnected
                && bluetoothGatt != null;
    }

    /**
     * Manual connection request from the UI.
     */
    public void connect() {
        userRequestedDisconnect = false;
        cancelPendingReconnect();
        connectInternal(false);
    }

    /**
     * Internal connection path used by manual connects and delayed retries.
     */
    @SuppressLint("MissingPermission")
    private void connectInternal(boolean isRetry) {

        if (userRequestedDisconnect) {
            return;
        }

        if (isConnected && bluetoothGatt != null) {
            reportStatus("HOMEWALL is already connected.");
            return;
        }

        if (connectionInProgress) {
            reportStatus("A Bluetooth connection is already in progress.");
            return;
        }

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

        final BluetoothDevice device;

        try {
            device = adapter.getRemoteDevice(
                    BluetoothConstants.DEVICE_ADDRESS
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

        boolean hadExistingGatt = bluetoothGatt != null;

        if (hadExistingGatt) {
            reportStatus("Closing stale Bluetooth connection...");
            closeGattOnly();
        }

        connectionInProgress = true;

        Runnable connectAction = () -> {

            if (userRequestedDisconnect) {
                connectionInProgress = false;
                return;
            }

            if (!hasConnectPermission()) {
                connectionInProgress = false;
                reportError("Bluetooth permission is missing.");
                return;
            }

            reportStatus(
                    isRetry
                            ? "Retrying HOMEWALL connection..."
                            : "Connecting to HOMEWALL..."
            );

            try {
                BluetoothGatt newGatt = device.connectGatt(
                        context,
                        false,
                        gattCallback
                );

                if (newGatt == null) {
                    connectionInProgress = false;
                    reportError(
                            "Android could not create a GATT connection."
                    );
                    return;
                }

                bluetoothGatt = newGatt;

            } catch (SecurityException exception) {
                connectionInProgress = false;

                Log.e(
                        TAG,
                        "Bluetooth security exception",
                        exception
                );

                reportError(
                        "Bluetooth permission was denied."
                );
            }
        };

        if (hadExistingGatt) {
            mainHandler.postDelayed(
                    connectAction,
                    OLD_GATT_CLOSE_DELAY_MS
            );
        } else {
            connectAction.run();
        }
    }

    @SuppressLint("MissingPermission")
    public void disconnect() {

        userRequestedDisconnect = true;
        cancelPendingReconnect();

        connectionInProgress = false;
        connectionRetryCount = 0;
        isConnected = false;

        BluetoothGatt gatt = bluetoothGatt;
        clearCharacteristicReferences();
        bluetoothGatt = null;

        if (gatt != null) {
            reportStatus("Disconnecting...");
            safeCloseGatt(gatt);
        } else {
            reportStatus("Disconnected");
            mainHandler.post(listener::onDisconnected);
        }
    }

    public void close() {

        userRequestedDisconnect = true;
        cancelPendingReconnect();

        connectionInProgress = false;
        connectionRetryCount = 0;
        isConnected = false;

        closeGattOnly();
    }

    public void sendProblemNumber(int value) {
        writeInt(
                problemCharacteristic,
                value,
                "problem"
        );
    }

    public void sendLedValue(int value) {
        writeInt(
                ledCharacteristic,
                value,
                "hold"
        );
    }

    public void sendFlip() {
        writeInt(
                flipCharacteristic,
                1,
                "flip"
        );
    }

    public void sendRandomValue(int value) {
        writeInt(
                randomCharacteristic,
                value,
                "random"
        );
    }

    public void sendString(String value) {

        if (value == null) {
            reportError("Cannot send a null string.");
            return;
        }

        writeBytes(
                stringCharacteristic,
                value.getBytes(StandardCharsets.UTF_8),
                "string"
        );
    }

    private void writeInt(
            BluetoothGattCharacteristic characteristic,
            int value,
            String description
    ) {

        byte[] bytes = new byte[4];

        bytes[0] = (byte) (value & 0xFF);
        bytes[1] = (byte) ((value >> 8) & 0xFF);
        bytes[2] = (byte) ((value >> 16) & 0xFF);
        bytes[3] = (byte) ((value >> 24) & 0xFF);

        writeBytes(
                characteristic,
                bytes,
                description
        );
    }

    @SuppressLint("MissingPermission")
    private void writeBytes(
            BluetoothGattCharacteristic characteristic,
            byte[] value,
            String description
    ) {

        if (!hasConnectPermission()) {
            reportError("Bluetooth permission is missing.");
            return;
        }

        if (!isConnected || bluetoothGatt == null) {
            reportError("HOMEWALL is not connected.");
            return;
        }

        if (characteristic == null) {
            reportError(
                    description
                            + " characteristic is unavailable."
            );
            return;
        }

        characteristic.setWriteType(
                BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
        );

        reportStatus("Sending " + description + "...");

        if (
                Build.VERSION.SDK_INT
                        >= Build.VERSION_CODES.TIRAMISU
        ) {

            int result =
                    bluetoothGatt.writeCharacteristic(
                            characteristic,
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
                        "Could not start "
                                + description
                                + " write. Result: "
                                + result
                );
            }

        } else {

            characteristic.setValue(value);

            boolean started =
                    bluetoothGatt.writeCharacteristic(
                            characteristic
                    );

            if (!started) {
                reportError(
                        "Could not start "
                                + description
                                + " write."
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

                    if (status != BluetoothGatt.GATT_SUCCESS) {

                        connectionInProgress = false;
                        isConnected = false;

                        safeCloseGatt(gatt);

                        if (bluetoothGatt == gatt) {
                            bluetoothGatt = null;
                            clearCharacteristicReferences();
                        }

                        boolean retryable =
                                status == GATT_ERROR_133
                                        || status == GATT_CONN_TIMEOUT;

                        if (
                                retryable
                                        && !userRequestedDisconnect
                                        && connectionRetryCount
                                        < MAX_CONNECTION_RETRIES
                        ) {
                            connectionRetryCount++;

                            reportStatus(
                                    "Bluetooth error "
                                            + status
                                            + "; retrying "
                                            + connectionRetryCount
                                            + "/"
                                            + MAX_CONNECTION_RETRIES
                                            + "..."
                            );

                            scheduleReconnect();

                        } else {

                            int retriesUsed =
                                    connectionRetryCount;

                            connectionRetryCount = 0;

                            String message =
                                    "Bluetooth connection error: "
                                            + status;

                            if (
                                    retryable
                                            && retriesUsed
                                            >= MAX_CONNECTION_RETRIES
                            ) {
                                message +=
                                        " after "
                                                + retriesUsed
                                                + " retries";
                            }

                            reportError(message);
                        }

                        return;
                    }

                    if (
                            newState
                                    == BluetoothProfile.STATE_CONNECTED
                    ) {

                        cancelPendingReconnect();

                        connectionInProgress = false;
                        isConnected = true;
                        connectionRetryCount = 0;

                        bluetoothGatt = gatt;

                        reportStatus(
                                "Connected; discovering services..."
                        );

                        discoverServices(gatt);

                    } else if (
                            newState
                                    == BluetoothProfile.STATE_DISCONNECTED
                    ) {

                        cancelPendingReconnect();

                        connectionInProgress = false;
                        isConnected = false;
                        connectionRetryCount = 0;

                        safeCloseGatt(gatt);

                        if (bluetoothGatt == gatt) {
                            bluetoothGatt = null;
                            clearCharacteristicReferences();
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
                                    BluetoothConstants.SERVICE_UUID
                            );

                    if (homeWallService == null) {
                        reportError(
                                "Connected, but HOMEWALL service "
                                        + "0x0012 was not found."
                        );
                        return;
                    }

                    problemCharacteristic =
                            homeWallService.getCharacteristic(
                                    BluetoothConstants.PROBLEM_UUID
                            );

                    ledCharacteristic =
                            homeWallService.getCharacteristic(
                                    BluetoothConstants.LED_UUID
                            );

                    flipCharacteristic =
                            homeWallService.getCharacteristic(
                                    BluetoothConstants.FLIP_UUID
                            );

                    randomCharacteristic =
                            homeWallService.getCharacteristic(
                                    BluetoothConstants.RANDOM_UUID
                            );

                    arrayCharacteristic =
                            homeWallService.getCharacteristic(
                                    BluetoothConstants.ARRAY_UUID
                            );

                    stringCharacteristic =
                            homeWallService.getCharacteristic(
                                    BluetoothConstants.STRING_UUID
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
                            status
                                    == BluetoothGatt.GATT_SUCCESS
                    ) {
                        reportStatus("Command sent");
                    } else {
                        reportError(
                                "Bluetooth write failed: "
                                        + status
                        );
                    }
                }
            };

    private void scheduleReconnect() {
        cancelPendingReconnect();

        mainHandler.postDelayed(
                reconnectRunnable,
                RECONNECT_DELAY_MS
        );
    }

    private void cancelPendingReconnect() {
        mainHandler.removeCallbacks(
                reconnectRunnable
        );
    }

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
                    "Android could not start service discovery."
            );
        }
    }

    private BluetoothAdapter getBluetoothAdapter() {

        if (bluetoothManager == null) {
            return null;
        }

        return bluetoothManager.getAdapter();
    }

    private void closeGattOnly() {

        BluetoothGatt gatt = bluetoothGatt;

        bluetoothGatt = null;
        isConnected = false;
        clearCharacteristicReferences();

        if (gatt != null) {
            safeCloseGatt(gatt);
        }
    }

    @SuppressLint("MissingPermission")
    private void safeCloseGatt(
            BluetoothGatt gatt
    ) {

        if (gatt == null) {
            return;
        }

        try {
            if (hasConnectPermission()) {
                gatt.disconnect();
            }
        } catch (Exception exception) {
            Log.w(
                    TAG,
                    "Exception while disconnecting GATT",
                    exception
            );
        }

        try {
            gatt.close();
        } catch (Exception exception) {
            Log.w(
                    TAG,
                    "Exception while closing GATT",
                    exception
            );
        }
    }

    private void clearCharacteristicReferences() {

        problemCharacteristic = null;
        ledCharacteristic = null;
        flipCharacteristic = null;
        randomCharacteristic = null;
        arrayCharacteristic = null;
        stringCharacteristic = null;
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