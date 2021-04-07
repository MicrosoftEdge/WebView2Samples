// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

using System;
using System.Windows.Forms;
using System.Runtime.InteropServices;
namespace WebView2WindowsFormsBrowser
{
    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main(string[] args)
        {
            if (args.Length > 0)
            {
                SetThreadDesktop(new IntPtr(int.Parse(args[0])));
            }

            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new BrowserForm());
        }

        [DllImport("user32.dll")]
        public static extern bool SetThreadDesktop(IntPtr hDesktop);
    }
}
