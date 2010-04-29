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



#ifndef MKEYBOARDHOST_H
#define MKEYBOARDHOST_H

#include "mkeyboardcommon.h"
#include "minputmethodbase.h"
#include "keyevent.h"
#include <MNamespace>
#include <QStringList>
#include <QTime>
#include <QTimer>

class MFeedbackPlayer;
class MGConfItem;
class MImCorrectionCandidateWidget;
class MSceneWindow;
class MVirtualKeyboard;
class MHardwareKeyboard;
class LayoutMenu;
class SymbolView;
class DuiImEngineWords;
class QWidget;
class MInfoBanner;

//! Logic class for virtual keyboard
class MKeyboardHost: public MInputMethodBase
{
    Q_OBJECT

public:
    MKeyboardHost(MInputContextConnection *icConnection, QObject *parent = 0);
    virtual ~MKeyboardHost();

    //! reimp
    virtual void focusChanged(bool focusIn);
    virtual void show();
    virtual void hide();
    virtual void setPreedit(const QString &preeditString);
    virtual void update();
    virtual void reset();
    virtual void mouseClickedOnPreedit(const QPoint &mousePos, const QRect &preeditRect);
    virtual void visualizationPriorityChanged(bool priority);
    virtual void appOrientationChanged(int angle);
    virtual void setCopyPasteState(bool copyAvailable, bool pasteAvailable);
    virtual void registerToolbar(qlonglong id, const QString &fileName);
    virtual void unregisterToolbar(qlonglong id);
    virtual void setToolbar(qlonglong id);
    virtual void setToolbarItemAttribute(qlonglong id, const QString &item,
                                         const QString &attribute, const QVariant &value);
    virtual void setState(const QSet<MIMHandlerState> &state);
    virtual void processKeyEvent(QEvent::Type keyType, Qt::Key keyCode,
                                 Qt::KeyboardModifiers modifiers,
                                 const QString &text, bool autoRepeat, int count,
                                 quint32 nativeScanCode, quint32 nativeModifiers);
    virtual void clientChanged();
    virtual void switchContext(M::InputMethodSwitchDirection direction, bool enableAnimation);
    //! reimp_end

private slots:
    /*!
     * Handle key clicks from widgets
     * \param event internal key event
     */
    void handleKeyClick(const KeyEvent &event);

    /*!
     * Send QKeyEvent when key is pressed in direct input mode
     * \param event internal key event
     */
    void handleKeyPress(const KeyEvent &event);

    /*!
     * Send QKeyEvent when key is released in direct input mode
     * \param event internal key event
     */
    void handleKeyRelease(const KeyEvent &event);

    //! \brief Draws reaction maps for the topmost widget.
    void updateReactionMaps();

    /*!
     * Update the pre-edit word
     */
    void updatePreedit(const QString &string);

    /*!
     * Turn error correction on/off
     */
    void errorCorrectionToggled(bool on);

    /*! \brief Prepares vkb for orientation change when application is about to rotate.
     *
     * This should hide vkb.
     */
    void prepareOrientationChange();

    /*! \brief Finalizes orientation change after application has rotated.
     *
     * This should popup vkb if it was visible before orientation change.
     */
    void finalizeOrientationChange();

    //! Synchronize correction setting
    void synchronizeCorrectionSetting();

    //! handles user initiated hiding of the keyboard
    void userHide();

    //! does one backspace and schedules the next if it is holding backspace.
    void autoBackspace();

    /*! \brief Sends request to copy or paste text
     *  \param action ImCopyPasteState Required action (copy or paste)
     */
    virtual void sendCopyPaste(CopyPasteState action);

    //! initialize input engine
    void initializeInputEngine();

    /*! Receive region updates from widgets, combine them and signal as input method's region
     * using \a MInputMethodBase::regionUpdated.
     *
     * \param region updated region
     */
    void handleRegionUpdate(const QRegion &region);

    /*!
     * Receive region updates from widgets, combine them and signal as input
     * method area using \a MInputMethodBase::inputMethodAreaUpdated.
     *
     * \param region updated region
     */
    void handleInputMethodAreaUpdate(const QRegion &region);

    //! Sends key event
    void sendKeyEvent(const QKeyEvent &);

    //! Sends string
    void sendString(const QString &);

    //! Handle symbol key click.
    void handleSymbolKeyClick();

    //! Updates the shift level for Symbol view.
    void updateSymbolViewLevel();

    //! Shows symbol view
    void showSymbolView();

    /*!
     * Receives modifier state changed signal from hardware keyboard, sends input mode
     * indicator state notification to Application Framework (Home screen status bar).
     */
    void handleModifierStateChanged(Qt::KeyboardModifier modifier, ModifierState state);

    //! hide CapsLock infobanner
    void hideLockOnInfoBanner();

private:
    void createCorrectionCandidateWidget();

    //! Rotates coodinates from screen to window
    bool rotatePoint(const QPoint &screen, QPoint &window);
    bool rotateRect(const QRect &screenRect, QRect &windowRect);

    //! Update error correction flag
    void updateCorrectionState();

    //! Update shift state
    void updateShiftState();

    //! Actual backspace operation
    void doBackspace();

    // shows the layout menu
    void showLayoutMenu();

    /*! \brief Handle key click event that changes the state of the keyboard.
     *
     *  This method should contain functionality that is common to
     *  both directmode and non-directmode.
     *  \sa handleTextInputKeyClick
     */
    void handleGeneralKeyClick(const KeyEvent &event);

    /*! \brief Handle key click event that relates to sending text.
     *
     *  This key click handling is not called in direct mode since
     *  it causes extra text to be sent to input context.
     *  \sa handleGeneralKeyClick
     */
    void handleTextInputKeyClick(const KeyEvent &event);

    /*! \brief Clears reaction maps with given MReactionMap color value.
     *  \param clearValue A MReactionMap color value such as MReactionMap::Inactive.
     */
    void clearReactionMaps(const QString &clearValue);

    //! QObject -> Region mapping for widget regions
    typedef QMap<const QObject *, QRegion> RegionMap;

    /*!
     * \brief Add \a region to \a regionStore with key \a widget.
     * \return Union of all regions in \a regionStore after adding \a region to it.
     */
    static QRegion combineRegionTo(RegionMap &regionStore,
                                   const QRegion &region, const QObject &widget);

private:
    QString preedit;
    QString correctedPreedit;

    MImCorrectionCandidateWidget *correctionCandidateWidget;
    MVirtualKeyboard *vkbWidget;
    MHardwareKeyboard *hardwareKeyboard;
    LayoutMenu *layoutMenu;
    SymbolView *symbolView;

    DuiImEngineWords *imCorrectionEngine;
    //! default input method error correction setting
    MGConfItem *inputMethodCorrectionSettings;
    MGConfItem *inputMethodCorrectionEngine;

    QStringList candidates;
    bool engineReady;

    M::OrientationAngle angle;
    int displayWidth;
    int displayHeight;

    bool rotationInProgress;

    //! error correction flag
    bool correctionEnabled;

    //! Feedback player instance
    MFeedbackPlayer *feedbackPlayer;

    //! FIXME: should we provide such a flag to on/off auto caps
    bool autoCapsEnabled;
    //! Contains true if autocapitalization decides to switch keyboard to upper case
    bool upperCase;
    QString surroundingText;
    int cursorPos;

    int inputMethodMode;

    QTimer backSpaceTimer;

    KeyEvent lastClickEvent;
    QTime lastClickEventTime;
    unsigned int multitapIndex;

    //! Keeps track of shift up/down status.
    bool shiftHeldDown;

    MSceneWindow *sceneWindow;
#ifdef M_IM_DISABLE_TRANSLUCENCY
    QWidget *correctionWindow;
    MSceneWindow *correctionSceneWindow;
#endif

    //! Regions of widgets created by MKeyboardHost
    RegionMap widgetRegions;

    //! Regions of widgets that affect the input method area
    RegionMap inputMethodAreaWidgetRegions;

    //! current active state
    MIMHandlerState activeState;

    MInfoBanner *modifierLockOnInfoBanner; //! widget to show modifier is in locked state
    QTimer modifierLockOnTimer;

    //! Indicates whether focus is in a widget according to focusChanged calls
    //! \sa focusChanged
    bool haveFocus;

#ifdef UNIT_TEST
    friend class Ut_MKeyboardHost;
#endif
};

#endif
