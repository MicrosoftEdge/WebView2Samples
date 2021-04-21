// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

using System.Runtime.InteropServices;
using System.Collections.Generic;
using System.Reflection;
using System;

namespace WebView2WpfBrowser
{
    // BridgeAddRemoteObject is a .NET object that implements IDispatch and works with AddRemoteObject.
    // See the AddRemoteObjectCmdExecute method that demonstrates how to use it from JavaScript.
#pragma warning disable CS0618
    // The.NET version of CoreWebView2.AddHostObjectToScript currently relies on the host object
    // implementing IDispatch and so uses the deprecated ClassInterfaceType.AutoDual feature of.NET.
    // This may change in the future, please see https://github.com/MicrosoftEdge/WebView2Feedback/issues/517  for more information
   [ClassInterface(ClassInterfaceType.AutoDual)]
#pragma warning restore CS0618
    [ComVisible(true)]
    public class AnotherRemoteObject
    {

        // Sample property.
        public string Prop { get; set; } = "AnotherRemoteObject.Prop";
    }

#pragma warning disable CS0618
    [ClassInterface(ClassInterfaceType.AutoDual)]
#pragma warning restore CS0618
    [ComVisible(true)]
    public class BridgeAddRemoteObject
    {
        // Sample function that takes a parameter.
        public string Func(string param)
        {
            return "BridgeAddRemoteObject.Func(" + param + ")";
        }

        // Sample function that takes no parameters.
        public string Func2()
        {
            return "BridgeAddRemoteObject.Func2()";
        }

        // Get type of an object.
        public string GetObjectType(object obj)
        {
            return obj.GetType().Name;
        }

        // Sample property.
        public string Prop { get; set; } = "BridgeAddRemoteObject.Prop";

        public AnotherRemoteObject AnotherObject { get; set; } = new AnotherRemoteObject();

        // Sample indexed property.
        [System.Runtime.CompilerServices.IndexerName("Items")]
        public string this[int index]
        {
            get { return m_dictionary[index]; }
            set { m_dictionary[index] = value; }
        }
        private Dictionary<int, string> m_dictionary = new Dictionary<int, string>();

        static public string scenarioHtml =
@"<!DOCTYPE html>

<button id='runTest'>Run Test</button>
<button id='method'>Method with param</button>
<button id='methodNoParam'>Method no param</button>
<button id='propertyGet'>Property Get</button>
<button id='propertySet'>Property Set</button>
<button id='indexerGet'>Get object[index]</button>
<button id='indexerSet'>Set object[index]</button>
<button id='indexerGetAlias'>Get Items[index]</button>
<button id='indexerSetAlias'>Set Items[index]</button>
<script>
    function getStack() {
        try {
            throw new Error('');
        } catch (error) {
            return error.stack;
        }
    }

    function valueToString(value) {
        return '(' + JSON.stringify(value) + ', ' + (typeof value) + ')';
    }
    async function assert(condition, text, message) {
        if (!condition) {
          alert('Assertion failed, ' + text + ': ' + message);
        } else {
          console.log('assert passed: ' + text);
        }
    }

    function assertEqual(actual, expected, text)
    {
        assert(expected === actual, text, ('Equality assertion failed. ' +
          'Expected ' + valueToString(expected) + ', ' +
          'Actual ' + valueToString(actual) + ', ' + getStack()));
    }
    document.getElementById('runTest').addEventListener('click', async () => {
        const bridge = chrome.webview.hostObjects.bridge;
        let expected_result = 'value1';
        bridge.Prop = expected_result;
        let result = await bridge.Prop;
        assertEqual(result, expected_result, 'property on bridge');
        const value2 = 'value2';
        result = await bridge.Func(value2);
        expected_result = 'BridgeAddRemoteObject.Func(' + value2 + ')';
        assertEqual(result, expected_result, 'method with parameter');
        result = await bridge.Func2();
        expected_result = 'BridgeAddRemoteObject.Func2()';
        assertEqual(result, expected_result, 'method with no parameter');

        const another_object = bridge.AnotherObject;
        another_object.Prop = value2;
        result = await another_object.Prop;
        expected_result = value2;
        assertEqual(result, expected_result, 'property on another_object');

        let index = 123;
        expected_result = 'aa';
        bridge[index] = expected_result;
        result = await bridge[index];
        assertEqual(result, expected_result, 'bridge[index]');
        index = 321;
        expected_result = 'bb';
        bridge.Items[index] = expected_result;
        result = await bridge.Items[index];
        assertEqual(result, expected_result, 'bridge.Items[index]');

        let resolved_bridge = await bridge;
        result = await bridge.GetObjectType(resolved_bridge);
        expected_result = 'BridgeAddRemoteObject';
        assertEqual(result, expected_result, 'type of resolved_bridge');
        result = await bridge.GetObjectType(bridge);
        expected_result = 'BridgeAddRemoteObject';
        assertEqual(result, expected_result, 'type of bridge');
        result = await bridge.GetObjectType(another_object);
        expected_result = 'AnotherRemoteObject';
        assertEqual(result, expected_result, 'type of another_object');

        alert('Test End');
    });
    document.getElementById('method').addEventListener('click', async () => {
        const bridge = chrome.webview.hostObjects.bridge;
        const result = await bridge.Func(prompt('Method parameter text', 'Method parameter text'));
        alert(result);
    });
    document.getElementById('methodNoParam').addEventListener('click', async () => {
        const bridge = chrome.webview.hostObjects.bridge;
        const result = await bridge.Func2();
        alert(result);
    });
    document.getElementById('propertyGet').addEventListener('click', async () => {
        const bridge = chrome.webview.hostObjects.bridge;
        const result = await bridge.Prop;
        alert(result);
    });
    document.getElementById('propertySet').addEventListener('click', async () => {
        const bridge = chrome.webview.hostObjects.bridge;
        bridge.Prop = prompt('Property text', 'Property text');
    });
    document.getElementById('indexerGet').addEventListener('click', async () => {
        const bridge = chrome.webview.hostObjects.bridge;
        const result = await bridge[1];
        alert(result);
    });
    document.getElementById('indexerSet').addEventListener('click', async () => {
        const bridge = chrome.webview.hostObjects.bridge;
        bridge[1] = prompt('Property text', 'Property text');
    });
    document.getElementById('indexerGetAlias').addEventListener('click', async () => {
        const bridge = chrome.webview.hostObjects.bridge;
        const result = await bridge.Items[12];
        alert(result);
    });
    document.getElementById('indexerSetAlias').addEventListener('click', async () => {
        const bridge = chrome.webview.hostObjects.bridge;
        bridge.Items[12] = prompt('Property text', 'Property text');
    });
</script>
";
    }
}
