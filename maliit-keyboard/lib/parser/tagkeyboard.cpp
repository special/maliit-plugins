// -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*-
/*
 * This file is part of Maliit Plugins
 *
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
 *
 * Contact: Mohammad Anwari <Mohammad.Anwari@nokia.com>
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this list
 * of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 * Neither the name of Nokia Corporation nor the names of its contributors may be
 * used to endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "tagkeyboard.h"

namespace MaliitKeyboard {

TagKeyboard::TagKeyboard(const QString &version,
                         const QString &title,
                         const QString &language,
                         const QString &catalog,
                         const bool autocapitalization)
    : m_version(version)
    , m_title(title)
    , m_language(language)
    , m_catalog(catalog)
    , m_autocapitalization(autocapitalization)
    , m_layouts()
{}

const QString TagKeyboard::version() const
{
    return m_version;
}

const QString TagKeyboard::title() const
{
    return m_title;
}

const QString TagKeyboard::language() const
{
    return m_language;
}

const QString TagKeyboard::catalog() const
{
    return m_catalog;
}

bool TagKeyboard::autocapitalization() const
{
    return m_autocapitalization;
}

const TagKeyboard::TagLayouts TagKeyboard::layouts() const
{
    return m_layouts;
}

void TagKeyboard::appendLayout(const TagLayoutPtr &layout)
{
    m_layouts.append(layout);
}


} // namespace MaliitKeyboard
