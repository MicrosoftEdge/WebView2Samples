// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "stdafx.h"

#include <string>

struct ClientCertificate
{
    ClientCertificate() {};

    ClientCertificate(const ClientCertificate& clientCertificate) {
        Subject = wil::make_cotaskmem_string(clientCertificate.Subject.get());
        DisplayName = wil::make_cotaskmem_string(clientCertificate.DisplayName.get());
        Issuer = wil::make_cotaskmem_string(clientCertificate.Issuer.get());
        ValidFrom = clientCertificate.ValidFrom;
        ValidTo = clientCertificate.ValidTo;
        CertificateKind = clientCertificate.CertificateKind;
    }

    wil::unique_cotaskmem_string Subject;
    wil::unique_cotaskmem_string DisplayName;
    wil::unique_cotaskmem_string Issuer;
    double ValidFrom;
    double ValidTo;
    PCWSTR CertificateKind;
};

// Constructing this struct will show a client certificate selection dialog and return when 
// the user dismisses it. If the user clicks the OK button, confirmed will be true with the 
// selected certificate.
struct ClientCertificateSelectionDialog
{
    ClientCertificateSelectionDialog(
        HWND parent,
        PCWSTR title,
        PCWSTR host,
        INT port,
        const std::vector<ClientCertificate> &clientCertificates);

    PCWSTR title;
    PCWSTR host;
    INT port;
    std::vector<ClientCertificate> clientCertificates;

    bool confirmed;
    int selectedItem;
};