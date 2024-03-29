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

#include "keyareaconverter.h"

#include "models/styleattributes.h"
#include "models/area.h"
#include "models/keyarea.h"
#include "models/key.h"
#include "logic/keyboardloader.h"

#include <QtCore>

namespace MaliitKeyboard {
namespace {

KeyArea createFromKeyboard(StyleAttributes *attributes,
                           const Keyboard &source,
                           Layout::Orientation orientation,
                           bool is_extended_keyarea = false)
{
    // An ad-hoc geometry updater that also uses styling information.
    KeyArea ka;
    Keyboard kb(source);

    if (not attributes) {
        qCritical() << __PRETTY_FUNCTION__
                    << "No style given, aborting.";
        return ka;
    }

    attributes->setStyleName(kb.style_name);

    Font font;
    font.setName(attributes->fontName(orientation));
    font.setSize(attributes->fontSize(orientation));
    font.setColor(attributes->fontColor(orientation));

    Font small_font(font);
    small_font.setSize(attributes->smallFontSize(orientation));

    static const QMargins bg_margins(attributes->keyBackgroundBorders());

    const qreal max_width(attributes->keyAreaWidth(orientation));
    const qreal key_height(attributes->keyHeight(orientation));
    const qreal margin = attributes->keyMargin(orientation);
    const qreal padding = attributes->keyAreaPadding(orientation);

    QPoint pos(0, 0);
    QVector<int> row_indices;
    int spacer_count = 0;
    qreal consumed_width = 0;


    for (int index = 0; index < kb.keys.count(); ++index) {
        row_indices.append(index);
        Key &key(kb.keys[index]);
        const KeyDescription &desc(kb.key_descriptions.at(index));
        int width = 0;
        pos.setY(key_height * desc.row);

        bool at_row_start((index == 0)
                          || (kb.key_descriptions.at(index - 1).row < desc.row));
        bool at_row_end((index + 1 == kb.keys.count())
                        || (index + 1 < kb.keys.count()
                            && kb.key_descriptions.at(index + 1).row > desc.row));

        if (desc.left_spacer || desc.right_spacer) {
            ++spacer_count;
        }

        width = attributes->keyWidth(orientation, desc.width);

        const qreal key_margin((at_row_start || at_row_end) ? margin + padding : margin * 2);

        Area area;
        area.setBackground(attributes->keyBackground(desc.style, KeyDescription::NormalState));
        area.setBackgroundBorders(bg_margins);
        area.setSize(QSize(width + key_margin, key_height));
        key.setArea(area);

        key.setOrigin(pos);
        key.setMargins(QMargins(at_row_start ? padding : margin, margin,
                                at_row_end   ? padding : margin, margin));

        const QString &text(key.label().text());
        key.rLabel().setFont(text.count() > 1 ? small_font : font);

        if (text.isEmpty()) {
            key.setIcon(attributes->icon(desc.icon,
                                    KeyDescription::NormalState));
        }

        pos.rx() += key.rect().width();

        if (at_row_end) {
            if (not is_extended_keyarea
                && spacer_count > 0 && pos.x() < max_width + 1) {
                const int spacer_width = qMax<int>(0, max_width - pos.x()) / spacer_count;
                pos.setX(0);
                int right_x = 0;

                Q_FOREACH (int row_index, row_indices) {
                    Key &k(kb.keys[row_index]);
                    const KeyDescription &d(kb.key_descriptions.at(row_index));

                    QRect r(k.rect());
                    QMargins m(k.margins());
                    int extra_width = 0;

                    if (d.left_spacer) {
                        m.setLeft(m.left() + spacer_width);
                        extra_width += spacer_width;
                    }

                    if (d.right_spacer) {
                        m.setRight(m.right() + spacer_width);
                        extra_width += spacer_width;
                    }

                    k.setMargins(m);

                    r.translate(right_x - r.left(), 0);
                    r.setWidth(r.width() + extra_width);
                    k.setOrigin(r.topLeft());
                    k.rArea().setSize(r.size());

                    right_x = r.x() + r.width();
                }
            }

            consumed_width = qMax<qreal>(consumed_width, key.rect().x()
                                                         + key.rect().width()
                                                         + padding);

            row_indices.clear();
            pos.setX(0);
            spacer_count = 0;
        }
    }

    Area area;
    area.setBackground(attributes->keyAreaBackground());
    area.setBackgroundBorders(attributes->keyAreaBackgroundBorders());
    area.setSize(QSize((is_extended_keyarea ? consumed_width : max_width),
                       pos.y() + key_height));

    ka.setArea(area);
    ka.setKeys(kb.keys);

    return ka;
}
}

KeyAreaConverter::KeyAreaConverter(StyleAttributes *attributes,
                                   KeyboardLoader *loader,
                                   const QPoint &anchor)
    : m_attributes(attributes)
    , m_loader(loader)
    , m_anchor(anchor)
{
    if (not attributes || not loader) {
        qFatal("Neither attributes nor loader can be null.");
    }
}

KeyAreaConverter::~KeyAreaConverter()
{}

KeyArea KeyAreaConverter::keyArea(Layout::Orientation orientation) const
{
    return createFromKeyboard(m_attributes, m_loader->keyboard(), orientation);
}

KeyArea KeyAreaConverter::nextKeyArea(Layout::Orientation orientation) const
{
    return createFromKeyboard(m_attributes, m_loader->nextKeyboard(), orientation);
}

KeyArea KeyAreaConverter::previousKeyArea(Layout::Orientation orientation) const
{
    return createFromKeyboard(m_attributes, m_loader->previousKeyboard(), orientation);
}

KeyArea KeyAreaConverter::shiftedKeyArea(Layout::Orientation orientation) const
{
    return createFromKeyboard(m_attributes, m_loader->shiftedKeyboard(), orientation);
}

KeyArea KeyAreaConverter::symbolsKeyArea(Layout::Orientation orientation,
                                         int page) const
{
    return createFromKeyboard(m_attributes, m_loader->symbolsKeyboard(page), orientation);
}

KeyArea KeyAreaConverter::deadKeyArea(Layout::Orientation orientation,
                                      const Key &dead) const
{
    return createFromKeyboard(m_attributes, m_loader->deadKeyboard(dead), orientation);
}

KeyArea KeyAreaConverter::shiftedDeadKeyArea(Layout::Orientation orientation,
                                             const Key &dead) const
{
    return createFromKeyboard(m_attributes, m_loader->shiftedDeadKeyboard(dead), orientation);
}

KeyArea KeyAreaConverter::extendedKeyArea(Layout::Orientation orientation,
                                          const Key &key) const
{
    return createFromKeyboard(m_attributes, m_loader->extendedKeyboard(key), orientation, true);
}

KeyArea KeyAreaConverter::numberKeyArea(Layout::Orientation orientation) const
{
    return createFromKeyboard(m_attributes, m_loader->numberKeyboard(), orientation);
}

KeyArea KeyAreaConverter::phoneNumberKeyArea(Layout::Orientation orientation) const
{
    return createFromKeyboard(m_attributes, m_loader->phoneNumberKeyboard(), orientation);
}

} // namespace MaliitKeyboard
