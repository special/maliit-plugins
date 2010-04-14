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



#ifndef MVIRTUALKEYBOARD_H
#define MVIRTUALKEYBOARD_H

#include "mkeyboardcommon.h"
#include "singlewidgetbuttonarea.h"
#include "mbuttonarea.h"
#include "layoutdata.h"
#include <mimhandlerstate.h>
#include <MWidget>
#include <QPixmap>
#include <QSharedPointer>
#include <QTimeLine>

class QGraphicsGridLayout;
class QGraphicsLinearLayout;
class QGraphicsWidget;
class MButton;
class MScalableImage;
class MSceneManager;
class MVirtualKeyboardStyleContainer;
class HorizontalSwitcher;
class KeyEvent;
class LayoutsManager;
class Notification;
class VKBDataKey;
class VkbToolbar;
class MImToolbar;
class DuiReactionMap;

/*!
  \class MVirtualKeyboard

  \brief The MVirtualKeyboard class provides interfaces for the usage of the
   virtual keyboard. The interfaces include hide/show/orientation of the keyboard.
   It also provides interfaces to get keystatus

*/
class MVirtualKeyboard : public MWidget
{
    Q_OBJECT

    friend class Ut_MVirtualKeyboard;
    friend class Ut_MKeyboardHost;

public:
    //! Shift key states
    typedef enum {
        ShiftOff,
        ShiftOn,
        ShiftLock
    } ShiftLevel;


    /*!
     * \brief Constructor for creating an virtual keyboard object.
     * \param parent Parent object.
     */
    MVirtualKeyboard(const LayoutsManager &layoutsManager, QGraphicsWidget *parent = 0);

    //! Destructor
    ~MVirtualKeyboard();

    //! \reimp
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);
    //! \reimp_end

    //! \brief Tells whether keyboard is opened and not in the middle of show or hide animation.
    bool isFullyVisible() const;

    /*!
     * \brief Method to get the language for the currently displayed layout
     *
     * Note that this is the real language found in the XML loaded based on a language
     * list entry (which may be different).
     * \return the language
     */
    QString layoutLanguage() const;

    /*!
     * \brief Get the language currently selected (from language list)
     *
     * This is the current language entry from the language list
     * \return the language currently selected (from language list)
     */
    QString selectedLanguage() const;

    //! Getter for style container
    MVirtualKeyboardStyleContainer &style();

    //! Sets keyboard type according text entry type, type matches M::TextContentType
    void setKeyboardType(const int type);

    // for unit tests
    //! Returns shift key status
    ShiftLevel shiftStatus() const;

    //! Characters defines word boundaries
    static const QString WordSeparators;

    /*!
     * \brief Return true if accurate mode is set for current layout, vice versa.
     * \return bool
     */
    bool isAccurateMode() const;

    void stopAccurateMode();

    /*! \brief Set copy/paste button state: hide it, show copy or show paste
     *  \param copyAvailable bool TRUE if text is selected
     *  \param pasteAvailable bool TRUE if clipboard content is not empty
     */
    virtual void setCopyPasteButton(bool copyAvailable, bool pasteAvailable);

    /*!
     * \brief Sets the status if there are some selection text.
     */
    void setSelectionStatus(bool);

    //! Prepare virtual keyboard for orientation change
    void prepareToOrientationChange();

    //! Finalize orientation change
    void finalizeOrientationChange();

    /*!
     * \brief Shows a custom toolbar with \a name.
     * Loads a custom toolbar according \a name, if successfuly loads,
     * the toolbar will be visible when virtual keyboard is shown.
     * \param name      Name of the custom toolbar.
     */
    void showToolbarWidget(const QString &name);

    /*!
     * \brief Hides all custom toolbars, this means they are removed from visible virtual keyboard.
     */
    void hideToolbarWidget();

    QRegion region(bool includeToolbar = true) const;

    /*!
     * \brief Shows or hides some keyboard's widgets depending on keyboard state.
     * OnScreen requires to show all keyboard components.
     * Any other value requires to hide everything except for toolbar.
     * \param newState actual state
     */
    void setKeyboardState(MIMHandlerState newState);

    /*!
     * \brief Returns current active state of the virtual keyboard.
     * The virtual keyboard has three kinds of modes, OnScreen, Hardware, and Accessory, \sa MIMHandlerState.
     * \sa setKeyboardState().
     */
    MIMHandlerState keyboardState() const;

    /*!
     * \brief Returns whether the symbol view is available for current layout.
     */
    bool symViewAvailable() const;

public slots:
    /*!
     * Method to switch level. Changes into next possible level.
     * Back to zero if maximum reached
     */
    void switchLevel();

    /*!
     * Method to set shift state
     */
    void setShiftState(ShiftLevel level);

    /*!
     * Method to Show the keyboard
     * \param fadeOnly just fade in if true, fade and slide otherwise
     */
    void showKeyboard(bool fadeOnly = false);

    /*!
     * Method to hide the keyboard
     * \param fadeOnly just fade out if true, fade and slide otherwise
     * \param temporary hide temporarily during screen rotation
     */
    void hideKeyboard(bool fadeOnly = false, bool temporary = false);

    void redrawReactionMaps();

    /*!
     * Method to change the orientation
     * \param orientation M::Orientation
     */
    void organizeContent(M::Orientation orientation);

    void setLanguage(int languageIndex);

    //! show virtual keyboard toolbar.
    void showToolbar();

    //! hide virtual keyboard toolbar.
    void hideToolbar();

    /*!
     * \brief Sets indicator button state according \a modifier and its \a state.
     */
    void setIndicatorButtonState(Qt::KeyboardModifier modifier, ModifierState state);

    //! Hide main keyboard layout
    void hideMainArea();

    //! Show main kayboard layout
    void showMainArea();

private slots:
    /*!
     * Handler for Right flick operation
     */
    void flickRightHandler();

    /*!
     * Handler for left flick operation
     */
    void flickLeftHandler();

    /*!
     * \brief Handler for upward flick operation
     * \param binding Key binding
     */
    void flickUpHandler(const KeyBinding *binding);

    /*!
     * Method to fade the vkb during transition
     */
    void fade(int);

    /*!
     * This function gets called when fading is finished
     */
    void showHideFinished();

    void languageReset();

    void onSectionSwitchStarting(int current, int next);

    void onSectionSwitched(QGraphicsWidget *previous, QGraphicsWidget *current);

    /*!
     * Send \a regionUpdated signal with the current region of this widget and
     * its children combined, unless \a suppressRegionUpdate has been used to
     * suppress updates.
     */
    void sendVKBRegion();

signals:
    /*!
     * \brief Emitted when key is pressed
     * Note that this happens also when user keeps finger down/mouse
     * button pressed and moves over another key (event is about the new key)
     * \param event key event
     */
    void keyPressed(const KeyEvent &event);

    /*!
     * \brief Emitted when key is released
     * Note that this happens also when user keeps finger down/mouse
     * button pressed and moves over another key (event is about the old key)
     * \param event key event
     */
    void keyReleased(const KeyEvent &event);

    /*!
     * \brief Emitted when user releases mouse button/lifts finger
     * Except when done on a dead key
     * \param event key event
     */
    void keyClicked(const KeyEvent &event);

    //! \see MInputMethodBase::regionUpdated()
    void regionUpdated(const QRegion &);

    //! This signal is emitted when input language is changed
    //! \param language this is always the language from XML file in unmodified form
    void languageChanged(const QString &language);

    //! Emitted when user hides the keyboard, e.g. by pressing the close button
    void userInitiatedHide();

    /*!
     * \brief This signal is emitted when copy/paste button is clicked
     * \param state CopyPasteState button action (copy or paste)
     */
    void copyPasteClicked(CopyPasteState action);

    //! Emitted when require a copy/paste action
    void copyPasteRequest(CopyPasteState);

    //! Emitted when require sending a keyevent
    void sendKeyEventRequest(const QKeyEvent &);

    //! Emitted when require sending a string
    void sendStringRequest(const QString &);

    //! Emitted when shift state is changed
    void shiftLevelChanged();

    //! Emitted after the widget has finished hiding.
    void hidden();

    //! Emitted when fully visible.
    void opened();

    //! Emitted when indicator button is clicked
    void indicatorClicked();

    //! Emitted when symbol view should be shown
    void showSymbolViewRequested();

private:
    const LayoutData *currentLayoutModel() const;

    /*!
     * Method to setup timeline
     */
    void setupTimeLine();

    /*!
     * \brief Create toolbar for virtual keyboard.
     */
    void createToolbar();

    /*!
     * Paint the reactive areas of the buttons
     *
     * This does not include the layout keys.
     * It paints the reactive areas for the buttons
     * in the bottom row of the keyboard as well as
     * the close/minimize/hide button.
     */
    void drawButtonsReactionMaps(DuiReactionMap *reactionMap, QGraphicsView *view);

    /*! This can be set true if it is known that we're going
     * to have multiple region updates simultaneously.
     * It prevents vkb to send its update region. sendVKBRegion()
     * is called automatically when updates are enabled again.
     */
    void suppressRegionUpdate(bool suppress);

    //! creates a switcher for qwerty layouts
    void createSwitcher();

    //! Reloads sections to switcher with current layout type and orientation
    void reloadSwitcherContent();

    //! Creates a new section widget of given keyboard/layout type and orientation.
    KeyButtonArea *createMainSectionView(const QString &language,
                                         LayoutData::LayoutType,
                                         M::Orientation orientation,
                                         QGraphicsWidget *parent = 0);

    // creates a new section widget
    KeyButtonArea *createSectionView(const QString &language,
                                     LayoutData::LayoutType layoutType,
                                     M::Orientation orientation,
                                     const QString &section,
                                     KeyButtonArea::ButtonSizeScheme sizeScheme,
                                     bool usePopup,
                                     QGraphicsWidget *parent = 0);

    // recreates keyboard widgets
    void recreateKeyboards();

    // makes new keybutton areas for number and phone number
    void recreateSpecialKeyboards();

    //! \brief Resets different components of vkb to their initial states.
    void resetState();

    //! \brief Creates vertical linear layout with correct styling to hold multiple keyboard sections.
    QGraphicsLinearLayout *createKeyAreaLayout(QGraphicsWidget *parent);

    //! \brief Returns height depending on current mode. It includes
    // toolbar only for hardware keyboard or whole widget for virtual
    // keyboard
    int actualHeight() const;

private:
    //! Keyboard state wrt. \a showKeyboard / \a hideKeyboard calls.
    enum Activity {
        Active,                 // After showKeyboard call
        Inactive,               // After hidekeyboard(..., false)
        TemporarilyInactive     // After hideKeyboard(..., true)
    };

    //! Slow/hide animation constants
    enum {
        ShowHideFrames = 100,
#ifdef SLOW_TRANSITIONS         // for easier debugging
        ShowHideTime = 750,
        ShowHideInterval = 50
#else
        ShowHideTime = 250,
        ShowHideInterval = 20
#endif
    };

    QGraphicsLinearLayout *mainLayout;

    //! Current layout level
    int currentLevel;

    //! Max number of layout levels
    int numLevels;

    //! \see Activity.  Note that e.g. call to hideKeyboard() marks keyboard
    //! immediately Inactive but that doesn't mean the keyboard is immediately
    //! hidden, so activity does not strictly match visibility.
    Activity activity;

    //! Current Style being used
    MVirtualKeyboardStyleContainer *styleContainer;

    //! Scene manager to get the device width and height
    MSceneManager *sceneManager;

    //! Shift key status
    int shiftLevel;

    // Vkb show hide time line
    QTimeLine showHideTimeline;

    LayoutData::LayoutType currentLayoutType;

    M::Orientation currentOrientation;

    QString currentLanguage;

    bool hideShowByFadingOnly;

    //! Used by suppressRegionUpdate & sendVKBRegion
    bool sendRegionUpdates;
    bool regionUpdateRequested;

    const LayoutsManager &layoutsMgr;

    //! Switcher for showing the main qwerty section.
    HorizontalSwitcher *mainKeyboardSwitcher;

    Notification *notification;

    //! Toolbar for virtual keyboard
    MImToolbar *imToolbar;

    QGraphicsWidget *numberKeyboard;
    QGraphicsLinearLayout *numberLayout;

    QGraphicsWidget *phoneNumberKeyboard;
    QGraphicsLinearLayout *phoneNumberLayout;

    QSharedPointer<QPixmap> backgroundPixmap;

    MIMHandlerState activeState;
};

#endif
