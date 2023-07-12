// Copyright(c) 2019 - 2022, KaoruXun All rights reserved.

using System;
using System.Runtime.CompilerServices;
using MetaDotManagedCore;
public class MetaDotManagedRuntime : BaseClass
{
    public override void OnAttach()
    {
        Log.METADOT_TRACE("TestingLayer attached");
        Log.METADOT_PROFILE_BEGIN_SESSION("startsession", "startSes.json");
    }

    private int e = 1;
    public override void OnUpdate()
    {

        base.OnUpdate();
        /*        if ((e++ & 255) == 0)
                    Log.METADOT_TRACE("updating c# FUCK ME FUCK ME");*/
    }

    public override void OnDetach()
    {
        Log.METADOT_TRACE("TestingLayer detached");
        Log.METADOT_PROFILE_END_SESSION();

    }
}
