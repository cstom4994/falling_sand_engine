// Copyright(c) 2019 - 2022, KaoruXun All rights reserved.

using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.ConstrainedExecution;
using System.Runtime.InteropServices;

namespace MetaDotManagedCore
{
    public class EntryHotSwap : Entry
    {


        private AppDomain currentDomain;
        private ProxyAssLoaderMarschal loader;
        private int ASS_CHECK_INTERVAL = 60;
        public override void SetAssemblyOutsideMode(bool b) => AssemblyLocator.ENABLE_OUTSIDE_SEARCH = b;
        public override void SetAssemblyLookupInterval(int i) => ASS_CHECK_INTERVAL = i;
        public override void ReloadAssembly()
        {
            try
            {
                ReloadDomainThrowing();
            }
            catch (Exception e)
            {
                Log.METADOT_ERROR("Failed to reload assembly due to error:");
                Log.METADOT_ERROR(e.ToString());
            }
        }
        public void LoadDomain()
        {
            var setup = new AppDomainSetup();
            setup.ApplicationBase = AssemblyLocator.ROOT_PATH;
            Log.METADOT_INFO("new appdomain in " + setup.ApplicationBase);

            currentDomain = AppDomain.CreateDomain(AssemblyLocator.DOMAIN_NAME, null, setup);
            Log.METADOT_INFO("New Domain created: " + currentDomain.ToString());

            currentDomain.AssemblyResolve += HandleAssemblyResolve;
            loader = (ProxyAssLoaderMarschal)currentDomain.CreateInstanceAndUnwrap(typeof(ProxyAssLoaderMarschal).Assembly.FullName, typeof(ProxyAssLoaderMarschal).FullName);
            loader.LoadFrom(AssemblyLocator.DOMAIN_PATH);

            Log.METADOT_TRACE("Host domain: " + AppDomain.CurrentDomain.FriendlyName);
            Log.METADOT_TRACE("child domain: " + currentDomain.FriendlyName);
            Log.METADOT_TRACE("Application base is: " + currentDomain.SetupInformation.ApplicationBase);

            loader.LoadLayers();
            loader.AttachLayers();
        }
        public void UnloadDomain()
        {
            if (currentDomain != null)
            {
                if (loader != null)
                {
                    loader.DetachLayers();
                    loader.UnloadLayers();
                }
                // Log.METADOT_TRACE("Unloading domain: " + currentDomain.FriendlyName);
                AppDomain.Unload(currentDomain);
            }
            currentDomain = null;
            loader = null;
        }
        public void ReloadDomainThrowing()
        {
            UnloadDomain();
            LoadDomain();
        }

        //file handling

        int searchModificationDivider = 0;
        private void divideCall(Action f)
        {
            searchModificationDivider++;
            if ((searchModificationDivider %= ASS_CHECK_INTERVAL) == 0 && ASS_CHECK_INTERVAL != 0)
                f();
        }



        //layer
        int counterToCheckAssLoad;//will wait for a while and then checks if managed was loaded if autoload is enabled

        public override void OnAttach()
        {
            AssemblyLocator.InitPaths();
            counterToCheckAssLoad = ASS_CHECK_INTERVAL * 2;
            Log.METADOT_TRACE("C# scripting attached: HotSwap");

            if (ASS_CHECK_INTERVAL == 0)//will be resolved in OnUpdate()
                LoadDomain();
        }
        public override void OnDetach()
        {
            if (currentDomain != null)
                AppDomain.Unload(currentDomain);
            Log.METADOT_TRACE("C# scripting detached");
        }
        public override void OnUpdate()
        {
            divideCall(() =>
            {
                if (AssemblyLocator.CheckForModification())
                {
                    Log.METADOT_INFO("Found newer dll - > copying");
                    UnloadDomain();
                    AssemblyLocator.CopyAssembly();
                    LoadDomain();
                }
            });
            if (loader != null)
                loader.UpdateLayers();

            if (counterToCheckAssLoad > 0)
            {
                if (--counterToCheckAssLoad == 0 && loader == null)
                {
                    Log.METADOT_WARN("Cannot find Managed.dll");
                }
            }


        }
        //loader
        static Assembly HandleAssemblyResolve(object source, ResolveEventArgs e)
        {
            Console.WriteLine("Cannot load {0} so i am loading", e.Name);
            return Assembly.Load(e.Name);
        }
        public override List<string> GetLayers()
        {
            var listOut = new List<string>();
            if (loader == null)
                return listOut;
            foreach (var layer in loader.layers)
                listOut.Add(layer.GetType().Name);
            return listOut;
        }
    }



}
