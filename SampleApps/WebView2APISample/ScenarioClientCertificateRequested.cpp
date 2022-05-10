// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "stdafx.h"

#include "ScenarioClientCertificateRequested.h"
#include "ClientCertificateSelectionDialog.h"
#include "CheckFailure.h"

using namespace Microsoft::WRL;

static PCWSTR NameOfCertificateKind(COREWEBVIEW2_CLIENT_CERTIFICATE_KIND kind);

ScenarioClientCertificateRequested::ScenarioClientCertificateRequested(AppWindow* appWindow)
    : m_appWindow(appWindow), m_webView(appWindow->GetWebView())
{
    // Register a handler for the `ClientCertificateRequested` event.
    // This example hides the default client certificate dialog and shows a custom dialog instead.
    // The dialog box displays mutually trusted certificates list and allows the user to select a certificate.
    // Selecting `OK` will continue the request with a certificate.
    // Selecting `CANCEL` will continue the request without a certificate.
    //! [ClientCertificateRequested2]
    m_webView2_5 = m_webView.try_query<ICoreWebView2_5>();
    if (m_webView2_5)
    {
        CHECK_FAILURE(
            m_webView2_5->add_ClientCertificateRequested(
                Callback<ICoreWebView2ClientCertificateRequestedEventHandler>(
                    [this](
                        ICoreWebView2* sender,
                        ICoreWebView2ClientCertificateRequestedEventArgs* args) {
                        auto showDialog = [this, args] {
                            wil::com_ptr<ICoreWebView2ClientCertificateCollection>
                                certificateCollection;
                            CHECK_FAILURE(args->get_MutuallyTrustedCertificates(&certificateCollection));

                            wil::unique_cotaskmem_string host;
                            CHECK_FAILURE(args->get_Host(&host));

                            INT port = FALSE;
                            CHECK_FAILURE(args->get_Port(&port));

                            UINT certificateCollectionCount;
                            CHECK_FAILURE(certificateCollection->get_Count(&certificateCollectionCount));

                            wil::com_ptr<ICoreWebView2ClientCertificate> certificate = nullptr;

                            if (certificateCollectionCount > 0)
                            {
                                ClientCertificate clientCertificate;
                                for (UINT i = 0; i < certificateCollectionCount; i++)
                                {
                                    CHECK_FAILURE(
                                        certificateCollection->GetValueAtIndex(i, &certificate));

                                    CHECK_FAILURE(certificate->get_Subject(&clientCertificate.Subject));

                                    CHECK_FAILURE(certificate->get_DisplayName(&clientCertificate.DisplayName));

                                    CHECK_FAILURE(certificate->get_Issuer(&clientCertificate.Issuer));

                                    COREWEBVIEW2_CLIENT_CERTIFICATE_KIND Kind;
                                    CHECK_FAILURE(
                                        certificate->get_Kind(&Kind));
                                    clientCertificate.CertificateKind = NameOfCertificateKind(Kind);

                                    CHECK_FAILURE(certificate->get_ValidFrom(&clientCertificate.ValidFrom));

                                    CHECK_FAILURE(certificate->get_ValidTo(&clientCertificate.ValidTo));

                                    clientCertificates_.push_back(clientCertificate);
                                }

                                // Display custom dialog box for the client certificate selection.
                                ClientCertificateSelectionDialog dialog(
                                    m_appWindow->GetMainWindow(), L"Select a Certificate for authentication", host.get(), port, clientCertificates_);

                                if (dialog.confirmed)
                                {
                                    int selectedIndex = dialog.selectedItem;
                                    if (selectedIndex >= 0)
                                    {
                                        CHECK_FAILURE(
                                            certificateCollection->GetValueAtIndex(selectedIndex, &certificate));
                                        // Continue with the selected certificate to respond to the server if `OK` is selected.
                                        CHECK_FAILURE(args->put_SelectedCertificate(certificate.get()));
                                    }
                                }
                                // Continue without a certificate to respond to the server if `CANCEL` is selected.
                                CHECK_FAILURE(args->put_Handled(TRUE));
                            }
                            else
                            {
                                // Continue without a certificate to respond to the server if certificate collection is empty.
                                CHECK_FAILURE(args->put_Handled(TRUE));
                            }
                        };

                        // Obtain a deferral for the event so that the CoreWebView2
                        // doesn't examine the properties we set on the event args and
                        // after we call the Complete method asynchronously later.
                        wil::com_ptr<ICoreWebView2Deferral> deferral;
                        CHECK_FAILURE(args->GetDeferral(&deferral));

                        // complete the deferral asynchronously.
                        m_appWindow->RunAsync([deferral, showDialog]() {
                            showDialog();
                            CHECK_FAILURE(deferral->Complete());
                            });

                        return S_OK;
                    })
                    .Get(),
                &m_ClientCertificateRequestedToken));

        MessageBox(
            nullptr, L"Custom Client Certificate selection dialog will be used next when WebView2 "
            L"is making a request to an HTTP server that needs a client certificate.",
            L"Client certificate selection", MB_OK);
    }
    else
    {
        FeatureNotAvailable();
    }
    //! [ClientCertificateRequested2]
}

static PCWSTR NameOfCertificateKind(COREWEBVIEW2_CLIENT_CERTIFICATE_KIND kind)
{
    switch (kind)
    {
    case COREWEBVIEW2_CLIENT_CERTIFICATE_KIND_SMART_CARD:
        return L"Smart Card";
    case COREWEBVIEW2_CLIENT_CERTIFICATE_KIND_PIN:
        return L"PIN";
    default:
        return L"Other";
    }
}

ScenarioClientCertificateRequested::~ScenarioClientCertificateRequested()
{
    if (m_webView2_5)
    {
        CHECK_FAILURE(
            m_webView2_5->remove_ClientCertificateRequested(m_ClientCertificateRequestedToken));
    }
}
