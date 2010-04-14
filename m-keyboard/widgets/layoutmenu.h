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



#ifndef LAYOUTMENU_H
#define LAYOUTMENU_H

#include <QObject>
#include <MNamespace>

class QGraphicsLinearLayout;
class QGraphicsWidget;
class MLayout;
class MLabel;
class MButton;
class MButtonGroup;
class MPopupList;
class MVirtualKeyboardStyleContainer;
class MDialog;
class MWidget;


/*!
 * \brief Virtual keyboard configuration dialog
 */
class LayoutMenu : public QObject
{
    Q_OBJECT
    friend class Ut_MKeyboardHost;

public:
    /*!
     * \brief Constructor
     * \param style style container
     * \param parent parent widget -- currently unused
     */
    LayoutMenu(MVirtualKeyboardStyleContainer *style, QGraphicsWidget *parent = 0);

    /*!
     * Destructor
     */
    ~LayoutMenu();

    //! Sets the list of available languages. These should
    //! be descriptive titles, not language codes.
    void setLanguageList(const QStringList &titles, int selected);

    //!  organize content when orientation is changed
    void organizeContent(M::Orientation orientation);

    //! Save state before rotation
    void save();

    //! Restore state after rotation
    void restore();

    /*!
     * enable/disable error correction
     */
    void enableErrorCorrection();
    void disableErrorCorrection();

    //! Whether layout menu is displayed
    bool isActive() const;

public slots:
    /*!
     * \brief Open the dialog if not open already.
     *
     * Returns only when user dismisses the dialog.
     */
    void show();

    /*!
     * Draw the reactive areas of this dialog
     */
    void redrawReactionMaps();


signals:
    //! Emitted when turn on/off error correction
    void errorCorrectionToggled(bool on);

    //! Emitted when layoutmenu is hidden
    void hidden();

    //! Emitted when interactive region of the menu has changed
    void regionUpdated(const QRegion &);

    //! Signals that user has selected a language. Index refers to
    //! a language in language list the menu was given.
    void languageSelected(int index);

private slots:
    void synchronizeErrorCorrection();

    void showLanguageList();

    //! Opens language settings page in mcontrolpanel.
    void openLanguageApplet();

    void visibleChangeHandler();

private:
    /*!
     * Method to Show animation
     */

    void setupShowAndHide();
    void getStyleValues();

    /*!
     * load language menu
     */
    void loadLanguageMenu();

    //! Getter for style container
    MVirtualKeyboardStyleContainer &style();

    //! Style attributes
    MVirtualKeyboardStyleContainer *styleContainer;

    QSize buttonSize;

    //! To check if menu is active
    bool active;

    //! to check if menu is active after rotation
    bool savedActive;

    //! baseSize
    QSize baseSize;

    //! Menu item
    MWidget *centralWidget;
    MLabel *titleLabel;
    MLabel *errorCorrectionLabel;
    MButton *errorCorrectionButton;
    MButtonGroup *errorCorectionButtonGroup;
    MLabel *layoutListLabel;
    MButton *layoutListHeader;
    MPopupList *layoutList;
    MLabel *languageSettingLabel;
    MButton *languageSettingButton;

    MDialog *keyboardOptionDialog;
    MWidget *menuWidget;
    MLayout *mainLayout;
    QGraphicsLinearLayout *correctionAndLanguageLandscapeLayout;
};

#endif