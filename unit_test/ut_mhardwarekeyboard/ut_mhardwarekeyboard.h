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



#ifndef UT_MVIRTUALKEYBOARD_H
#define UT_MVIRTUALKEYBOARD_H

#include <QObject>
#include <QSharedPointer>
#include <QtTest/QTest>

class MApplication;
class MHardwareKeyboard;

class Ut_MHardwareKeyboard : public QObject
{
    Q_OBJECT
private:
    MApplication *app;
    MHardwareKeyboard *m_hkb;

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testSetModifierState();
    void testRedirectKey();
    void testModifierInNumberContentType();
    void testModifierInPhoneNumberContentType();
    void testReset();
    void testAutoCaps();
    void testMultiKeys();
    void testHandleIndicatorButtonClick();
    void testSymbolPlusCharKeys();

private:
    void testModifierRedirectKey(Qt::Key modifierKey);
    void testSymbolRedirectKey();
};

#endif
