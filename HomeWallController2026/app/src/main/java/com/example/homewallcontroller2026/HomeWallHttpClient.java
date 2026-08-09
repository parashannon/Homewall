package com.example.homewallcontroller2026;

import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class HomeWallHttpClient {

    private static final String TAG = "HomeWallHTTP";

    private static final String RECENT_CLIMBS_URL =
            "http://192.168.4.46:8080/api/recent-climbs?limit=100";

    private final ExecutorService executor =
            Executors.newSingleThreadExecutor();

    private final Handler mainHandler =
            new Handler(Looper.getMainLooper());

    public interface Listener {
        void onClimbsLoaded(List<RecentClimb> climbs);

        void onError(String message);
    }

    public void loadRecentClimbs(Listener listener) {
        executor.execute(() -> {
            HttpURLConnection connection = null;

            try {
                URL url = new URL(RECENT_CLIMBS_URL);

                connection = (HttpURLConnection) url.openConnection();
                connection.setRequestMethod("GET");
                connection.setConnectTimeout(5000);
                connection.setReadTimeout(5000);
                connection.setUseCaches(false);
                connection.setRequestProperty(
                        "Accept",
                        "application/json"
                );

                int responseCode = connection.getResponseCode();

                if (responseCode < 200 || responseCode >= 300) {
                    throw new Exception(
                            "Pi returned HTTP " + responseCode
                    );
                }

                String responseText =
                        readEntireStream(connection.getInputStream());

                List<RecentClimb> climbs =
                        parseRecentClimbs(responseText);

                mainHandler.post(
                        () -> listener.onClimbsLoaded(climbs)
                );

            } catch (Exception exception) {
                Log.e(TAG, "Failed to load climbs", exception);

                String message = exception.getMessage();

                if (message == null || message.trim().isEmpty()) {
                    message = exception.getClass().getSimpleName();
                }

                String finalMessage = message;

                mainHandler.post(
                        () -> listener.onError(finalMessage)
                );

            } finally {
                if (connection != null) {
                    connection.disconnect();
                }
            }
        });
    }

    private String readEntireStream(InputStream inputStream)
            throws Exception {

        StringBuilder result = new StringBuilder();

        try (
                BufferedReader reader = new BufferedReader(
                        new InputStreamReader(
                                inputStream,
                                StandardCharsets.UTF_8
                        )
                )
        ) {
            String line;

            while ((line = reader.readLine()) != null) {
                result.append(line);
            }
        }

        return result.toString();
    }

    private List<RecentClimb> parseRecentClimbs(String jsonText)
            throws Exception {

        JSONObject root = new JSONObject(jsonText);
        JSONArray climbsArray = root.getJSONArray("climbs");

        List<RecentClimb> climbs = new ArrayList<>();

        for (int index = 0; index < climbsArray.length(); index++) {
            JSONObject climbObject =
                    climbsArray.getJSONObject(index);

            String name = climbObject.getString("name");
            int level = climbObject.getInt("level");
            String timestamp =
                    climbObject.optString("timestamp", "");

            climbs.add(
                    new RecentClimb(
                            name,
                            level,
                            timestamp
                    )
            );
        }

        return climbs;
    }

    public void shutdown() {
        executor.shutdownNow();
    }
}