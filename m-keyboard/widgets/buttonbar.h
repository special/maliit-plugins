/* * This file is part of m-keyboard *
 *
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 * All rights reserved.
 * Contact: Nokia Corporation (directui@nokia.com)
 *
 * If you have questions regarding the use of this file, please contact
 * Nokia at directui@nokia.com.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * and appearing in the file LICENSE.LGPL included in the packaging
 * of this file.
 */



#ifndef BUTTONBAR_H
#define BUTTONBAR_H

#include "buttonbarstyle.h"

#include <MStylableWidget>

#include <QList>

class MButton;
class QGraphicsLinearLayout;

/*!
    \brief ButtonBar displays buttons in a horizontal bar.

    Buttons can be delimited with a divider. Divider hides away when button is down
    or checked.

    Buttons are not owned by this class, though they are reparented.
*/
class ButtonBar : public MStylableWidget
{
    Q_OBJECT
public:
    ButtonBar(bool useDividers, QGraphicsItem *parent = 0);
    virtual ~ButtonBar();

    //! \brief Number of buttons added to this widget.
    int count() const;

    //! \brief Inserts a \a button to specified \a index.
    void insert(int index, MButton *button);

    //! \brief Appends \a button to the right end of the row.
    void append(MButton *button);

    //! \brief Removes the specified \a button from the widget.
    void remove(MButton *button);

    //! \brief Removes all buttons from the widget.
    void clear();

    //! \brief Retrieves a \a button at specific \a index.
    MButton *buttonAt(int index) const;

    //! \brief Check if \a button has been added.
    bool contains(const MButton *button) const;

    //! \brief Returns index of specified \a button or -1
    // if button was not found
    int indexOf(const MButton *button) const;

protected:
    //! \reimp
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void styleChanged();
    //! \reimp_end

private:
    typedef QList<MButton *> ButtonList;

    QGraphicsLinearLayout &mainLayout;

    const bool useDividers;

    ButtonList buttons;

    M_STYLABLE_WIDGET(ButtonBarStyle)
};

#endif // BUTTONBAR_H