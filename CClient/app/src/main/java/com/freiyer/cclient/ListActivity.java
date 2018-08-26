package com.freiyer.cclient;

import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Build;
import android.os.Handler;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.AdapterView;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.RadioButton;
import android.widget.SimpleAdapter;
import android.widget.TextView;
import android.widget.Toast;

import java.net.Socket;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

public class ListActivity extends AppCompatActivity {

    CClientApplication app;
    EditText editText;
    SimpleAdapter adapter;
    List<Map<String, String>> list;
    ListView listView;
    RadioButton touchscreenButton;
    SharedPreferences pref;
    SharedPreferences.Editor editor;
    Map<String, String> serverList;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_list);

        app = (CClientApplication)getApplication();
        editText = findViewById(R.id.editText);
        listView = findViewById(R.id.listView);
        touchscreenButton = findViewById(R.id.touchscreenButton);

        list = new ArrayList<>();
        serverList = new HashMap<>();
        adapter = new SimpleAdapter(
                this,
                list,
                android.R.layout.simple_list_item_2,
                new String[] {"addr", "name"},
                new int[] {android.R.id.text2, android.R.id.text1}
        );
        listView.setAdapter(adapter);
        listView.setOnItemClickListener(new ListView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> adapterView, View view, int i, long l) {
                editText.setText(((Map<String, String>)adapterView.getItemAtPosition(i)).get("addr"));
            }
        });
        listView.setOnItemLongClickListener(new ListView.OnItemLongClickListener() {
            @Override
            public boolean onItemLongClick(AdapterView<?> adapterView, View view, int i, long l) {
                Map<String, String> map = ((Map<String, String>)adapterView.getItemAtPosition(i));
                final String name = map.get("name");
                final String addr = map.get("addr");
                String fullName = name + "(" + addr + ")";
                new AlertDialog.Builder(ListActivity.this)
                        .setMessage(String.format(getString(R.string.message), name + "(" + addr + ")"))
                        .setTitle(getString(R.string.alert))
                        .setIcon(android.R.drawable.ic_dialog_alert)
                        .setPositiveButton(getString(R.string.positive), new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialogInterface, int i) {
                                Log.d(CClientApplication.LOG_TAG, addr);
                                serverList.remove(addr);
                                updateServerlist();
                            }
                        })
                        .setNegativeButton(getString(R.string.negative), new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialogInterface, int i) {

                            }
                        })
                        .setCancelable(false)
                        .create().show();

                return false;
            }
        });
        editor = (pref = getPreferences(Context.MODE_PRIVATE)).edit();
    }

    @Override
    protected void onResume() {
        super.onResume();
        editText.setText(pref.getString("tmpIp", ""));

        Set<String> set = pref.getStringSet("savedList", new HashSet<String>());
        for (String s : set) {
            String[] split = s.split(":");
            serverList.put(split[0], split[1]);
        }
        updateServerlist();
    }

    private void updateServerlist() {
        list.clear();
        for(String key : serverList.keySet()) {
            Map<String, String> map = new HashMap<>();
            map.put("addr", key);
            map.put("name", serverList.get(key));
            list.add(map);
        }
        adapter.notifyDataSetChanged();
    }

    @Override
    protected void onPause() {
        super.onPause();
        editor.putString("tmpIp", editText.getText().toString());
        Set<String> set = new HashSet<>();
        for(String key : serverList.keySet()) {
            set.add(key + ":" + serverList.get(key));
        }
        editor.putStringSet("savedList", set);
        editor.commit();
    }

    protected void onButtonClick(View view) {

        app.setListener(new CClientApplication.OnClientConnectListener() {
            @Override
            public void onSuccess(CClientApplication app, Socket socket) {
                Log.d("CClientApplication", app.getServerName());
                serverList.put(editText.getText().toString(), app.getServerName());
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Intent intent = new Intent(getApplicationContext(), MainActivity.class);
                        startActivity(intent);
                    }
                });
            }

            @Override
            public void onFail(CClientApplication app) {
                Log.d("CClientApplication", "FAIL");
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Log.d(CClientApplication.LOG_TAG, getString(R.string.failed));
                        Toast.makeText(getApplicationContext(), getString(R.string.failed), Toast.LENGTH_SHORT).show();
                    }
                });
            }
        });

        int mode;
        if(touchscreenButton.isChecked())
            mode = CClientApplication.MODE_TOUCHSCREEN;
        else
            mode = CClientApplication.MODE_TOUCHPAD;

        app.initializeNetwork(editText.getText().toString(), mode);
    }
}
