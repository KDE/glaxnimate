// SPDX-FileCopyrightText: 2021 Mattia Basaglia <dev@dragon.best>
// SPDX-License-Identifier: GPL-3.0-or-later
package org.kde.glaxnimate;

import android.app.Notification;
import android.content.Intent;
import android.os.Bundle;

import org.qtproject.qt.android.bindings.QtActivity;


public class GlaxnimateActivity extends QtActivity
{
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        Intent intent = getIntent();
        if ( intent != null )
        {
              String action = intent.getAction();
              if ( action != null )
                process_intent(intent);
        }
    }

    @Override
    public void onNewIntent(Intent i)
    {
        process_intent(i);
        super.onNewIntent(i);
    }

    private void process_intent(Intent i)
    {
        if ( i.getAction() == Intent.ACTION_VIEW )
            GlaxnimateActivity.openIntent(i.getData().toString());
    }

    private static native void openIntent(String uri);
}
