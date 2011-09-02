/*
 * This file is part of meego-keyboard 
 *
 * Copyright (C) 2010-2011 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
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

 */
#include "flickgesture.h"
#include "flickgesturerecognizer.h"
#include "borderpanrecognizer.h"
#include "mimabstractkeyarea.h"
#include "mimabstractkeyarea_p.h"
#include "mimkeyvisitor.h"
#include "mimreactionmap.h"
#include "mkeyboardhost.h"
#include "mimabstractpopup.h"
#include "layoutpanner.h"

#include <MCancelEvent>
#include <MFeedback>
#include <MGConfItem>
#include <QEvent>
#include <QGraphicsSceneMouseEvent>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QCoreApplication>
#include <QDebug>
#include <mtimestamp.h>

#ifdef unix
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

namespace
{
    // This GConf item defines whether multitouch is enabled or disabled
    const char *const MultitouchSettings = "/meegotouch/inputmethods/multitouch/enabled";

    // Minimal distance (in px) for touch point from key button edge.
    const int CorrectionDistanceThreshold = 2;

    // Used for error correction:
    QPoint gAdjustedPositionForCorrection = QPoint();

    QString toString(const QPointF &p, const QString &separator = ", ")
    {
        return QString("%1%2%3").arg(p.x())
                                .arg(separator)
                                .arg(p.y());
    }

    QString toString(const QRectF &r, const QString separator = ", ")
    {
        return QString("%1%2%3%4%5").arg(toString(r.topLeft(), separator))
                                    .arg(separator)
                                    .arg(r.width())
                                    .arg(separator)
                                    .arg(r.height());
    }

    QString toString(Qt::TouchPointState state)
    {
        switch(state) {
        default:
            return "n/a";

        case Qt::TouchPointPressed:
            return "pressed";

        case Qt::TouchPointMoved:
            return "moved";

        case Qt::TouchPointStationary:
            return "stationary";

        case Qt::TouchPointReleased:
            return "released";
        }
    }

    QString timeStamp()
    {
#ifdef unix
        struct timeval tv;
        gettimeofday(&tv, 0);
        return  QString("%1.%2").arg(tv.tv_sec).arg(tv.tv_usec);
#else
        return QTime::currentTime().toString();
#endif
    }
}

M::InputMethodMode MImAbstractKeyAreaPrivate::InputMethodMode;

MImAbstractKeyAreaPrivate::MImAbstractKeyAreaPrivate(const LayoutData::SharedLayoutSection &newSection,
                                                     MImAbstractKeyArea *owner)
    : q_ptr(owner),
      currentLevel(0),
      popup(0),
      wasGestureTriggered(false),
      feedbackSliding(MImReactionMap::Sliding),
      section(newSection),
      allowedHorizontalFlick(true),
      longPressTouchPointId(0),
      longPressTouchPointIsPrimary(false),
      enabledPanning(true),
      lastTouchEvent(QEvent::TouchEnd)
{
}

MImAbstractKeyAreaPrivate::~MImAbstractKeyAreaPrivate()
{
}

void MImAbstractKeyAreaPrivate::click(MImAbstractKey *key, const KeyContext &keyContext)
{
    Q_Q(MImAbstractKeyArea);

    if (!key || !key->enabled()) {
        return;
    }

    MImKeyVisitor::SpecialKeyFinder deadFinder(MImKeyVisitor::FindDeadKey);
    MImAbstractKey::visitActiveKeys(&deadFinder);
    MImAbstractKey *deadKey = deadFinder.deadKey();

    if (!key->isDeadKey()) {
        emit q->keyClicked(key, keyContext);
        if (!key->isShiftKey()) {
            q->unlockDeadKeys(deadKey);
        }
    } else if (key == deadKey) {
        q->unlockDeadKeys(deadKey);
    } else {
        // Deselect previous dead key, if any:
        if (deadKey) {
            deadKey->setSelected(false);
        }

        // key is the new deadkey:
        key->setSelected(true);
        q->updateKeyModifiers(key->label().at(0));
    }
}

void MImAbstractKeyAreaPrivate::handleFlickGesture(int direction,
                                                   Qt::GestureState state)
{
    Q_Q(MImAbstractKeyArea);

    if (InputMethodMode == M::InputMethodModeDirect) {
        return;
    }

    // Any non-upward flick gesture, complete or not, resets active keys etc.
    if (!wasGestureTriggered
        && (state != Qt::NoGesture)
        && direction != FlickGesture::Up) {

        if (popup) {
            popup->cancel();
        }

        MImAbstractKey *const lastActiveKey = MImAbstractKey::lastActiveKey();
        if (lastActiveKey && lastActiveKey->state() == MImAbstractKey::Pressed) {
            MImKeyVisitor::SpecialKeyFinder finder;
            MImAbstractKey::visitActiveKeys(&finder);
            const bool hasActiveShiftKeys = (finder.shiftKey() != 0);
            const KeyContext context(hasActiveShiftKeys || isUpperCase());
            emit q->keyCancelled(lastActiveKey, context);
        }

        MImKeyVisitor::KeyAreaReset reset;
        MImAbstractKey::visitActiveKeys(&reset);

        touchPointRecords.clear();
        longPressTimer.stop();
        wasGestureTriggered = true;
    }

    if (state == Qt::GestureFinished) {

        switch (direction) {
        case FlickGesture::Down:
            emit q->flickDown();
            break;

        case FlickGesture::Up:
            // Flick up not used.
            break;

        default:
            return;
        }
    }
}

void MImAbstractKeyAreaPrivate::handleGesture(const PanGesture &gesture)
{
    Q_Q(MImAbstractKeyArea);

    if (InputMethodMode == M::InputMethodModeDirect) {
        return;
    }

    if (!allowedHorizontalFlick) {
        return;
    }

    // Any non-upward flick gesture, complete or not, resets active keys etc.
    if (!wasGestureTriggered
        && (gesture.state() != Qt::NoGesture)) {

        if (popup) {
            popup->cancel();
        }

        MImAbstractKey *const lastActiveKey = MImAbstractKey::lastActiveKey();
        if (lastActiveKey && lastActiveKey->state() == MImAbstractKey::Pressed) {
            MImKeyVisitor::SpecialKeyFinder finder;
            MImAbstractKey::visitActiveKeys(&finder);
            const bool hasActiveShiftKeys = (finder.shiftKey() != 0);
            const KeyContext context(hasActiveShiftKeys || isUpperCase());
            emit q->keyCancelled(lastActiveKey, context);
        }

        //FIXME: should we do it here?
        MImKeyVisitor::KeyAreaReset reset;
        MImAbstractKey::visitActiveKeys(&reset);

        longPressTimer.stop();
        wasGestureTriggered = true;
    }


    switch (gesture.state()) {
    case Qt::GestureStarted:
        LayoutPanner::instance().tryPan(gesture.direction(),
                                        gesture.startPosition());
        q->ungrabGesture(FlickGestureRecognizer::sharedGestureType());
        break;
    case Qt::GestureUpdated:
    case Qt::GestureFinished:
        break;
    case Qt::GestureCanceled:
        q->grabGesture(FlickGestureRecognizer::sharedGestureType());
        break;
    case Qt::NoGesture:
        break;
    }
}

void MImAbstractKeyAreaPrivate::handleTouchEvent(QTouchEvent *event)
{
    Q_Q(MImAbstractKeyArea);

    lastTouchEvent = *event;

    if (!q->isVisible()) {
        return;
    }

    AutoCommitBehaviour autoCommitState = AllowAutoCommit;

    foreach (const QTouchEvent::TouchPoint &tp, event->touchPoints()) {

        switch (tp.state()) {
        case Qt::TouchPointPressed:
            touchPointPressed(tp, autoCommitState);

            // Disallow so multiple presses in the same touch event
            // cannot commit each other.
            autoCommitState = DisallowAutoCommit;
            break;

        case Qt::TouchPointMoved:
            touchPointMoved(tp);
            break;

        case Qt::TouchPointReleased:
            touchPointReleased(tp);
            break;
        default:
            break;
        }
    }

    // Stuck key guard. Reset all touch point counts.
    if (event->type() == QEvent::TouchEnd) {
        cancelAllKeys();
    }
}

void MImAbstractKeyAreaPrivate::touchPointPressed(const QTouchEvent::TouchPoint &tp,
                                                  AutoCommitBehaviour autoCommitState)
{
    Q_Q(MImAbstractKeyArea);

    mTimestamp("MImAbstractKeyArea", "start");
    wasGestureTriggered = false;
    mostRecentTouchPositions.insert(tp.id(), tp.pos());
    touchPointRecords[tp.id()] = TouchPointRecord(); // Clear record for this touch point.

    // Gestures only slow down in speed typing mode:
    if (isInSpeedTypingMode(true)) {
        idleVkbTimer.stop();
        // TODO: check how expensive gesture (un)grabbing is:
        q->ungrabGesture(FlickGestureRecognizer::sharedGestureType());
        q->ungrabGesture(BorderPanRecognizer::sharedGestureType());
    }

    const QPoint pos = q->correctedTouchPoint(tp.scenePos());
    MImAbstractKey *key = q->keyAt(pos);

    if (q->debugTouchPoints) {
        q->logTouchPoint(tp, key);
    }

    if (!key || !key->enabled()) {
        longPressTimer.stop();
        mTimestamp("MImAbstractKeyArea", "end");
        return;
    }

    // Try to commit currently active key before activating new key:
    MImAbstractKey *const lastActiveKey = MImAbstractKey::lastActiveKey();
    MImKeyVisitor::SpecialKeyFinder finder;
    MImAbstractKey::visitActiveKeys(&finder);
    const bool hasActiveShiftKeys = (finder.shiftKey() != 0);

    if (autoCommitState == AllowAutoCommit
        && q->style()->commitPreviousKeyOnPress()
        && lastActiveKey
        && lastActiveKey != key
        && lastActiveKey->enabled()
        && lastActiveKey->isNormalKey()
        && lastActiveKey->touchPointCount() > 0) {
        // TODO: play release sound? Potentially confusing to user, who
        // might still press this key.

        emit q->keyClicked(lastActiveKey,
                           KeyContext(hasActiveShiftKeys || isUpperCase(),
                                      QString(),
                                      tp.scenePos(),
                                      gAdjustedPositionForCorrection,
                                      false, 0, source));
        lastActiveKey->resetTouchPointCount();
    }

    // Hit new key.
    TouchPointRecord &rec = touchPointRecords[tp.id()];
    rec.setHitKey(key);

    if (rec.touchPointEnteredKey() && rec.key()->touchPointCount() == 1) {
        q->updatePopup(rec.key());
        longPressTouchPointId = tp.id();
        longPressTouchPointIsPrimary = tp.isPrimary();
        longPressTimer.start(q->style()->longPressTimeout());

        emit q->keyPressed(rec.key(),
                           KeyContext(hasActiveShiftKeys || isUpperCase(),
                                      finder.deadKey() ? finder.deadKey()->label() : QString(),
                                      tp.scenePos(), QPoint(), tp.isPrimary(),
                                      0, source));
    }
    mTimestamp("MImAbstractKeyArea", "end");
}

void MImAbstractKeyAreaPrivate::touchPointMoved(const QTouchEvent::TouchPoint &tp)
{
    Q_Q(MImAbstractKeyArea);

    mostRecentTouchPositions.insert(tp.id(), tp.pos());

    if (wasGestureTriggered) {
        longPressTimer.stop();
        return;
    }

    if (tp.scenePos() == tp.lastScenePos()) {
        return;
    }

    mTimestamp("MImAbstractKeyArea", "start");

    const QPoint pos = q->correctedTouchPoint(tp.scenePos());

    {
        // Modify and store the touch point record.
        TouchPointRecord &rec = touchPointRecords[tp.id()];
        rec.setHitKey(gravitationalKeyAt(pos, rec.hasGravity() ? rec.key() : 0));
    }

    // Take copy since keyarea may get reset when emitting key events.
    const TouchPointRecord rec = touchPointRecords[tp.id()];

    MImKeyVisitor::SpecialKeyFinder finder;
    MImAbstractKey::visitActiveKeys(&finder);
    const bool hasActiveShiftKeys = (finder.shiftKey() != 0);

    if (rec.touchPointEnteredKey() && rec.key()->touchPointCount() == 1) {
        // Reaction map cannot discover when we move from one key
        // (= reactive area) to another
        // slot is called asynchronously to get screen update as fast as possible
        QMetaObject::invokeMethod(&feedbackSliding, "play", Qt::QueuedConnection);

        longPressTouchPointId = tp.id();
        longPressTouchPointIsPrimary = tp.isPrimary();
        longPressTimer.start(q->style()->longPressTimeout());
        emit q->keyPressed(rec.key(),
                           KeyContext(hasActiveShiftKeys || isUpperCase(),
                                      finder.deadKey() ? finder.deadKey()->label() : QString(),
                                      tp.scenePos(), QPoint(), tp.isPrimary(),
                                      0, source));
    }

    if (rec.touchPointLeftKey() && rec.previousKey()->touchPointCount() == 0) {
        emit q->keyReleased(rec.previousKey(),
                            KeyContext(hasActiveShiftKeys || isUpperCase(),
                                       finder.deadKey() ? finder.deadKey()->label() : QString(),
                                       QPointF(), QPoint(), false, 0, source));
    }

    if (rec.key() != rec.previousKey()) {
        q->updatePopup(rec.key());
    }

    if (!rec.key()) {
        longPressTimer.stop();
    }

    if (q->debugTouchPoints) {
        q->logTouchPoint(tp, rec.key(), rec.previousKey());
    }

    mTimestamp("MImAbstractKeyArea", "end");
}

void MImAbstractKeyAreaPrivate::touchPointReleased(const QTouchEvent::TouchPoint &tp)
{
    Q_Q(MImAbstractKeyArea);

    mostRecentTouchPositions.insert(tp.id(), tp.pos());

    if (wasGestureTriggered) {
        return;
    }
    mTimestamp("MImAbstractKeyArea", "start");

    idleVkbTimer.start(q->style()->idleVkbTimeout());

    const QPoint pos = q->correctedTouchPoint(tp.scenePos());

    MImKeyVisitor::SpecialKeyFinder finder;
    MImAbstractKey::visitActiveKeys(&finder);

    {
        // Modify and store the touch point record.
        TouchPointRecord &rec = touchPointRecords[tp.id()];

        // If key has changed on release this will first increase its touch point count.
        rec.setHitKey(gravitationalKeyAt(pos, rec.hasGravity() ? rec.key() : 0));
    }

    // Key context before release
    KeyContext keyContext((finder.shiftKey() != 0) || isUpperCase(),
                          finder.deadKey() ? finder.deadKey()->label() : QString(),
                          tp.scenePos(),
                          gAdjustedPositionForCorrection,
                          false, 0, source);

    // Take copy since keyarea may get reset when emitting key events.
    const TouchPointRecord rec = touchPointRecords[tp.id()];

    if (rec.key()
        && rec.key()->decreaseTouchPointCount()
        && !rec.touchPointEnteredKey() // This is already missing keyPressed so don't emit keyReleased.
        && rec.key()->touchPointCount() == 0) {
        longPressTimer.stop();

        emit q->keyReleased(rec.key(), keyContext);

        // Update key context after release.
        MImAbstractKey::visitActiveKeys(&finder);
        KeyContext clickContext = keyContext;
        clickContext.upperCase = (finder.shiftKey() != 0) || isUpperCase();
        clickContext.accent = finder.deadKey() ? finder.deadKey()->label() : QString();

        click(rec.key(), clickContext);
    }

    if (rec.touchPointLeftKey() && rec.previousKey()->touchPointCount() == 0) {
        // Use the same key context as before release.
        emit q->keyReleased(rec.previousKey(), keyContext);
    }

    // We're finished with this touch point, inform popup:
    if (popup) {
        popup->cancel();
    }

    longPressTimer.stop();

    if (q->debugTouchPoints) {
        q->logTouchPoint(tp, rec.key(), rec.previousKey());
    }
    touchPointRecords.remove(tp.id());
    mTimestamp("MImAbstractKeyArea", "end");
}

QTouchEvent::TouchPoint
MImAbstractKeyAreaPrivate::fromMouseEvent(const QGraphicsSceneMouseEvent *event,
                                          MouseEventToTouchPointOption option)
{
    Qt::TouchPointState state;
    switch (event->type()) {
    case QEvent::GraphicsSceneMousePress:
    case QEvent::GraphicsSceneMouseDoubleClick: // arrives on press
        state = Qt::TouchPointPressed;
        break;
    case QEvent::GraphicsSceneMouseMove:
        state = Qt::TouchPointMoved;
        break;
    case QEvent::GraphicsSceneMouseRelease:
        state = Qt::TouchPointReleased;
        break;
    default:
        qWarning("Trying to convert mouse event of invalid type to a touch point.");
        state = Qt::TouchPointStationary;
        break;
    }

    QTouchEvent::TouchPoint tp(0);
    tp.setState(state | Qt::TouchPointPrimary);
    tp.setPos(event->pos());
    tp.setScenePos(event->scenePos());

    if (option == CopyAllMembers) {
        tp.setLastPos(event->lastPos());
        tp.setLastScenePos(event->lastScenePos());
    } else {
        tp.setLastPos(event->pos());
        tp.setLastScenePos(event->scenePos());
    }

    return tp;
}

bool
MImAbstractKeyAreaPrivate::multiTouchEnabled()
{
    static bool gConfRead = false;
    static bool touchEventsAccepted = false;
    if (!gConfRead) {
        touchEventsAccepted = MGConfItem(MultitouchSettings).value().toBool();
        gConfRead = true;
    }
    return touchEventsAccepted;
}

MImAbstractKey *MImAbstractKeyAreaPrivate::gravitationalKeyAt(const QPoint &pos,
                                                              MImAbstractKey *gravityKey) const
{
    Q_Q(const MImAbstractKeyArea);

    MImAbstractKey *key = 0;

    if (gravityKey) {
        const qreal hGravity = q->style()->touchpointHorizontalGravity();
        const qreal vGravity = q->style()->touchpointVerticalGravity();

        QRectF gravityArea = gravityKey->buttonBoundingRect().adjusted(-hGravity, -vGravity,
                                                                        hGravity, vGravity);
        if (gravityArea.contains(pos)) {
            key = gravityKey;
        }
    }

    if (!key) {
        key = q->keyAt(pos);
    }

    // Check whether error correction needs updated position:
    if (key) {
        QPoint &ec = gAdjustedPositionForCorrection;
        const QRectF &br = key->buttonRect();

        ec.setX(qBound<int>(br.left() + CorrectionDistanceThreshold,
                            pos.x(),
                            br.right() - CorrectionDistanceThreshold));

        ec.setY(qBound<int>(br.top() + CorrectionDistanceThreshold,
                            pos.y(),
                            br.bottom() - CorrectionDistanceThreshold));
    }

    return key;
}

bool MImAbstractKeyAreaPrivate::isInSpeedTypingMode(bool restartTimers)
{
    Q_Q(MImAbstractKeyArea);

    return ((restartTimers ? lastTouchPointPressEvent.restart()
                           : lastTouchPointPressEvent.elapsed()) < q->style()->idleVkbTimeout());
}

void MImAbstractKeyAreaPrivate::switchStyleMode()
{
    Q_Q(MImAbstractKeyArea);

    // TODO: rename to syncStyleMode (with layout)?
    if (!q->style()->syncStyleModeWithKeyCount()) {
        return;
    }

    q->style().setCurrentMode(section->styleName());
}

void MImAbstractKeyAreaPrivate::cancelAllKeys()
{
    Q_Q(MImAbstractKeyArea);

    MImKeyVisitor::KeyAreaReset keyAreaReset;
    keyAreaReset.setKeyParentItem(q);

    // Use default value for KeyContext parameter of keyCancelled. It's not used.
    QObject::connect(&keyAreaReset, SIGNAL(keyReleased(const MImAbstractKey *)),
                     q, SIGNAL(keyCancelled(const MImAbstractKey *)));

    MImAbstractKey::visitActiveKeys(&keyAreaReset);
}

// actual class implementation
MImAbstractKeyArea::MImAbstractKeyArea(MImAbstractKeyAreaPrivate *privateData,
                                       QGraphicsWidget *parent)
    : MStylableWidget(parent),
      mRelativeKeyBaseWidth(0),
      debugTouchPoints(false),
      d_ptr(privateData)
{
    Q_D(MImAbstractKeyArea);

    // By default multi-touch is disabled
    if (d->multiTouchEnabled()) {
        setAcceptTouchEvents(true);
    }

    d->lastTouchPointPressEvent.restart();
    grabGesture(FlickGestureRecognizer::sharedGestureType());
    if (d->enabledPanning)  {
        grabGesture(BorderPanRecognizer::sharedGestureType());
    }

    d->longPressTimer.setSingleShot(true);
    d->idleVkbTimer.setSingleShot(true);

    connect(&d->longPressTimer, SIGNAL(timeout()),
            this, SLOT(handleLongKeyPressed()));

    connect(&d->idleVkbTimer, SIGNAL(timeout()),
            this, SLOT(handleIdleVkb()));

    connect(MTheme::instance(), SIGNAL(themeChangeCompleted()),
            this, SLOT(onThemeChangeCompleted()),
            Qt::UniqueConnection);
}

MImAbstractKeyArea::~MImAbstractKeyArea()
{
    delete d_ptr;
}

void MImAbstractKeyArea::init()
{
    Q_D(MImAbstractKeyArea);

    d->switchStyleMode();
    debugTouchPoints = style()->debugTouchPoints();

    if (BorderPanRecognizer *instance = BorderPanRecognizer::instance()) {
        instance->setTimeout(style()->panGestureTimeout());
        instance->setStartThreshold(style()->panGestureStartThreshold());
        instance->setFinishThreshold(style()->panGestureFinishThreshold());
        instance->setInitialMovement(style()->panGestureInitialMovement());
    }
}

void MImAbstractKeyArea::setInputMethodMode(M::InputMethodMode inputMethodMode)
{
    MImAbstractKeyAreaPrivate::InputMethodMode = inputMethodMode;
}

qreal MImAbstractKeyArea::relativeKeyBaseWidth() const
{
    return mRelativeKeyBaseWidth;
}

const LayoutData::SharedLayoutSection &MImAbstractKeyArea::sectionModel() const
{
    Q_D(const MImAbstractKeyArea);

    return d->section;
}

void MImAbstractKeyArea::updatePopup(MImAbstractKey *key)
{
    Q_D(MImAbstractKeyArea);

    if (!d->popup) {
        return;
    }

    if (!key || !isVisible()) {
        d->popup->cancel();
        return;
    }

    const QRectF &buttonRect = key->buttonRect();
    // IM FW guarantees that a scene position matches with
    // screen position, so we can use mapToScene to calculate screen position:
    const QPoint pos = mapToScene(buttonRect.topLeft()).toPoint();

    MImKeyVisitor::SpecialKeyFinder finder(MImKeyVisitor::FindDeadKey);
    MImAbstractKey::visitActiveKeys(&finder);

    d->popup->updatePos(buttonRect.topLeft(), pos, buttonRect.toRect().size());
    d->popup->handleKeyPressedOnMainArea(key,
                                          KeyContext(d->isUpperCase(),
                                                     finder.deadKey() ? finder.deadKey()->label() : QString(),
                                                     QPointF(), QPoint(), false, 0, d->source));
}

int MImAbstractKeyArea::maxColumns() const
{
    Q_D(const MImAbstractKeyArea);

    return d->section->maxColumns();
}

int MImAbstractKeyArea::rowCount() const
{
    Q_D(const MImAbstractKeyArea);

    return d->rowCount();
}

void
MImAbstractKeyArea::handleVisibilityChanged(bool visible)
{
    Q_D(MImAbstractKeyArea);

    if (!visible) {
        if (d->popup) {
            d->popup->setVisible(false);
        }

        MImKeyVisitor::SpecialKeyFinder finder(MImKeyVisitor::FindDeadKey);
        MImAbstractKey::visitActiveKeys(&finder);

        unlockDeadKeys(finder.deadKey());

        foreach (const MImAbstractKey *key, keys()) {
            // We know there is a special case for backspace
            // need keyCancelled signal
            if (key->isBackspaceKey()
                && key->state() == MImAbstractKey::Pressed) {
                emit keyCancelled(key, KeyContext());
            }
        }

        MImKeyVisitor::KeyAreaReset keyAreaReset;
        keyAreaReset.setKeyParentItem(this);
        MImAbstractKey::visitActiveKeys(&keyAreaReset);
    }
}

void
MImAbstractKeyArea::switchLevel(int level)
{
    Q_D(MImAbstractKeyArea);

    if (level != d->currentLevel) {
        d->currentLevel = level;

        // Update uppercase / lowercase
        MImKeyVisitor::SpecialKeyFinder finder(MImKeyVisitor::FindDeadKey);
        MImAbstractKey::visitActiveKeys(&finder);

        updateKeyModifiers(finder.deadKey() ? finder.deadKey()->label().at(0) : '\0');

        update();
    }
}

int MImAbstractKeyArea::level() const
{
    Q_D(const MImAbstractKeyArea);

    return d->currentLevel;
}

void MImAbstractKeyArea::setShiftState(ModifierState /*newShiftState*/)
{
    // Empty default implementation
}

void MImAbstractKeyArea::resizeEvent(QGraphicsSceneResizeEvent *event)
{
    const int newWidth = static_cast<int>(event->newSize().width());

    if (newWidth != static_cast<int>(event->oldSize().width())) {
        updateKeyGeometries(newWidth);
    }
}

void MImAbstractKeyArea::mousePressEvent(QGraphicsSceneMouseEvent *ev)
{
    Q_D(MImAbstractKeyArea);

    if (d->multiTouchEnabled()) {
        return;
    }

    d->touchPointPressed(d->fromMouseEvent(ev));
}

void MImAbstractKeyArea::mouseMoveEvent(QGraphicsSceneMouseEvent *ev)
{
    Q_D(MImAbstractKeyArea);

    if (d->multiTouchEnabled()) {
        return;
    }

    d->touchPointMoved(d->fromMouseEvent(ev));
}

void MImAbstractKeyArea::mouseReleaseEvent(QGraphicsSceneMouseEvent *ev)
{
    Q_D(MImAbstractKeyArea);

    if (d->multiTouchEnabled()) {
        return;
    }

    d->touchPointReleased(d->fromMouseEvent(ev));
}

QVariant MImAbstractKeyArea::itemChange(GraphicsItemChange change, const QVariant &value)
{
    switch (change) {
    case QGraphicsItem::ItemVisibleChange:
        handleVisibilityChanged(value.toBool());
        break;

    default:
        break;
    }

    return QGraphicsItem::itemChange(change, value);
}

void MImAbstractKeyArea::grabMouseEvent(QEvent */*event*/)
{
    Q_D(MImAbstractKeyArea);

    // If mimabstractkeyarea is hidden without mouseReleaseEvent
    // the enabled <flicked> would stay true if mouse
    // grab is obtained again without mousePressEvent.
    // This would ignore mouseReleaseEvent and would not cause keyClicked.
    d->wasGestureTriggered = false;

    const qreal ScalingFactor = style()->flickGestureThresholdRatio();
    const int HorizontalThreshold = static_cast<int>(boundingRect().width() * ScalingFactor);
    const int VerticalThreshold = static_cast<int>(boundingRect().height() * ScalingFactor);
    const int Timeout = style()->flickGestureTimeout();

    FlickGestureRecognizer::instance()->setFinishThreshold(HorizontalThreshold, VerticalThreshold);
    FlickGestureRecognizer::instance()->setStartThreshold(HorizontalThreshold / 2, VerticalThreshold / 2);
    FlickGestureRecognizer::instance()->setTimeout(Timeout);
}

void MImAbstractKeyArea::ungrabMouseEvent(QEvent *)
{
    Q_D(MImAbstractKeyArea);

    // Make sure popup can respond to mouse grab removal:
    if (d->popup) {
        d->popup->cancel();
    }

    d->longPressTimer.stop();
}

void MImAbstractKeyArea::cancelEvent(MCancelEvent *)
{
    reset();
}

bool MImAbstractKeyArea::event(QEvent *ev)
{
    Q_D(MImAbstractKeyArea);

    bool eaten = false;
    QString start, end;
    start = QString("%1|start").arg(ev->type());
    end = QString("%1|end").arg(ev->type());
    mTimestamp("MImAbstractKeyArea", start);

    if (ev->type() == QEvent::Gesture) {
        const Qt::GestureType flickGestureType = FlickGestureRecognizer::sharedGestureType();
        FlickGesture *flickGesture = static_cast<FlickGesture *>(static_cast<QGestureEvent *>(ev)->gesture(flickGestureType));

        if (flickGesture) {
            d->handleFlickGesture(flickGesture->direction(), flickGesture->state());
            eaten = true;
        }

        const Qt::GestureType panGestureType = BorderPanRecognizer::sharedGestureType();
        PanGesture *panGesture = static_cast<PanGesture *>(static_cast<QGestureEvent *>(ev)->gesture(panGestureType));

        if (panGesture) {
            d->handleGesture(*panGesture);
            eaten = true;
        }
    } else if (ev->type() == QEvent::TouchBegin
               || ev->type() == QEvent::TouchUpdate
               || ev->type() == QEvent::TouchEnd) {
        d->handleTouchEvent(static_cast<QTouchEvent*>(ev));
        eaten = true;
    }

    const bool result = eaten || MWidget::event(ev);

    mTimestamp("MImAbstractKeyArea", end);
    return result;
}

void MImAbstractKeyArea::unlockDeadKeys(MImAbstractKey *deadKey)
{
    if (!deadKey || !deadKey->isDeadKey()) {
        return;
    }

    deadKey->setSelected(false);
    deadKey->resetTouchPointCount();
    updateKeyModifiers('\0');
}

void MImAbstractKeyArea::hidePopup()
{
    Q_D(MImAbstractKeyArea);

    if (!d->popup) {
        return;
    }

    d->popup->setVisible(false);
}

void MImAbstractKeyArea::drawReactiveAreas(MReactionMap *,
                                           QGraphicsView *)
{
    // Empty default implementation. Geometries of buttons are known by derived classes.
}

void MImAbstractKeyArea::setPopup(MImAbstractPopup *popup)
{
    Q_D(MImAbstractKeyArea);

    if (popup != d->popup) {
        delete d->popup;
        d->popup = popup;

        if (d->popup) {
            d->popup->setMainArea(this);
        }
    }
}

const MImAbstractPopup *MImAbstractKeyArea::popup() const
{
    Q_D(const MImAbstractKeyArea);

    return d->popup;
}

void MImAbstractKeyArea::logTouchPoint(const QTouchEvent::TouchPoint &tp,
                                       const MImAbstractKey *key,
                                       const MImAbstractKey *lastKey) const
{
    static bool headerWritten = false;
    QTextStream &out = MKeyboardHost::instance()->touchPointLog();

    if (!headerWritten) {
        out << "time (sec.msec)\t"
            << "tp_id\t"
            << "tp_state\t"
            << "start_x\t" << "start_y\t"
            << "last_x\t" << "last_y\t"
            << "current_x\t" << "current_y\t"
            << "center_x\t" << "center_y\t"
            << "delta_x\t" << "delta_y\t"
            << "label\t"
            << "label_last\t"
            << "br_x\t" << "br_y\t" << "br_w\t" << "br_h\n";
        headerWritten = true;
    }

    out << timeStamp() << "\t"
        << tp.id() << "\t"
        << toString(tp.state()) << "\t"
        << toString(tp.startScenePos(), "\t") << "\t"
        << toString(tp.lastScenePos(), "\t") << "\t"
        << toString(tp.scenePos(), "\t") << "\t"
        << (key ? toString(mapRectToScene(key->buttonBoundingRect()).center(), "\t")
                : "n/a\t") << "\t"
        << (key ? toString(tp.scenePos() - mapRectToScene(key->buttonBoundingRect()).center(), "\t")
                : "n/a\t") << "\t"
        << (key ? key->label() : "n/a") << "\t"
        << (lastKey ? lastKey->label() : "n/a") << "\t"
        << (key ? toString(mapRectToScene(key->buttonBoundingRect()), "\t")
                : "n/a\t\t\t") << "\n";
}

void MImAbstractKeyArea::updateKeyModifiers(const QChar &accent)
{
    Q_D(MImAbstractKeyArea);

    // We currently don't allow active dead key level changing. If we did,
    // we should update activeDeadKey level before delivering its accent to
    // other keys.
    bool shift = (d->currentLevel == 1);
    modifiersChanged(shift, accent);
}

void MImAbstractKeyArea::modifiersChanged(bool,
                                          const QChar &)
{
    // Empty default implementation
}

void MImAbstractKeyArea::enableHorizontalFlick(bool enable)
{
    Q_D(MImAbstractKeyArea);

    d->allowedHorizontalFlick = enable;
}

void MImAbstractKeyArea::onThemeChangeCompleted()
{
    Q_D(MImAbstractKeyArea);

    // TODO: update all other CSS attributes that are mapped to members.
    d->switchStyleMode();
    updateKeyGeometries(size().width());
}

const MImAbstractKeyAreaStyleContainer &MImAbstractKeyArea::baseStyle() const
{
    return style();
}

void MImAbstractKeyArea::handleLongKeyPressed()
{
    Q_D(MImAbstractKeyArea);
    MImAbstractKey *const lastActiveKey = MImAbstractKey::lastActiveKey();

    if (!lastActiveKey) {
        return;
    }

    MImKeyVisitor::SpecialKeyFinder finder(MImKeyVisitor::FindDeadKey);
    MImAbstractKey::visitActiveKeys(&finder);

    const QString accent = (finder.deadKey() ? finder.deadKey()->label()
                                           : QString());

    KeyContext keyContext(d->isUpperCase(), accent,
                          mapToScene(d->mostRecentTouchPositions.value(d->longPressTouchPointId)),
                          QPoint(), false, 0, d->source);
    keyContext.touchPointId = d->longPressTouchPointId;
    keyContext.isFromPrimaryTouchPoint = d->longPressTouchPointIsPrimary;

    if (d->popup) {
        // Reset state of long pressed key if necessary
        if (MImAbstractPopup::ResetPressedKey
                == d->popup->handleLongKeyPressedOnMainArea(lastActiveKey, keyContext)) {
            lastActiveKey->resetTouchPointCount();
        }
    }

    emit longKeyPressed(lastActiveKey, keyContext);
}

void MImAbstractKeyArea::handleIdleVkb()
{
    Q_D(MImAbstractKeyArea);
    grabGesture(FlickGestureRecognizer::sharedGestureType());
    if (d->enabledPanning) {
        grabGesture(BorderPanRecognizer::sharedGestureType());
    }
}

void MImAbstractKeyArea::reset()
{
    Q_D(MImAbstractKeyArea);

    // Handle shift before ungrab because that changes key state
    bool shiftLatchedOrLocked(false);

    foreach (const MImAbstractKey *key, keys()) {
        if (key->isShiftKey()) {
            shiftLatchedOrLocked = key->modifiers();
        }

        if ((key->isShiftKey() || key->isBackspaceKey())
            && key->state() == MImAbstractKey::Pressed) {
            emit keyCancelled(key, KeyContext());
        }
    }

    if (scene()->mouseGrabberItem() == this) {
        // Ungrab mouse explicitly since we probably used grabMouse() to get it.
        ungrabMouse();
    }

    if (d->popup) {
        d->popup->cancel();
    }

    d->touchPointRecords.clear();

    MImKeyVisitor::SpecialKeyFinder deadFinder(MImKeyVisitor::FindDeadKey);
    MImAbstractKey::visitActiveKeys(&deadFinder);
    unlockDeadKeys(deadFinder.deadKey());

    MImKeyVisitor::KeyAreaReset keyAreaReset;
    keyAreaReset.setKeyParentItem(this);
    MImAbstractKey::visitActiveKeys(&keyAreaReset);
    modifiersChanged(shiftLatchedOrLocked);
    update();
}

QPoint MImAbstractKeyArea::correctedTouchPoint(const QPointF &scenePos) const
{
    QPointF pos = mapFromScene(scenePos);

    if (pos.y() >= baseStyle()->touchpointVerticalOffset()) {
        pos.ry() -= baseStyle()->touchpointVerticalOffset();
    }

    return pos.toPoint();
}

QRectF MImAbstractKeyArea::correctedReactionRect(const QRectF &originalRect) const
{
    QRectF rect = originalRect;
    const qreal offset = baseStyle()->touchpointVerticalOffset();

    if (rect.top() >= offset) {
        rect.setTop(rect.top() + offset);
    }
    const qreal newBottom(rect.bottom() + offset);
    if (newBottom <= size().height()) {
        rect.setBottom(newBottom);
    }

    return rect;
}

bool MImAbstractKeyArea::contains(const MImAbstractKey *key) const
{
    return (key && keys().contains(key));
}

void MImAbstractKeyArea::setSource(KeyEvent::Source source)
{
    Q_D(MImAbstractKeyArea);
    d->source = source;
}

MImAbstractKeyAreaPrivate::TouchPointRecord::TouchPointRecord()
    : m_key(0),
      m_previousKey(0),
      keyHasGravity(false)
{
}

MImAbstractKey *MImAbstractKeyAreaPrivate::TouchPointRecord::key() const
{
    return m_key;
}

MImAbstractKey *MImAbstractKeyAreaPrivate::TouchPointRecord::previousKey() const
{
    return m_previousKey;
}

void MImAbstractKeyAreaPrivate::TouchPointRecord::setHitKey(MImAbstractKey *key)
{
    m_previousKey = m_key;

    if (key == m_key) {
        return;
    }

    m_key = key;

    // If key is disabled or touch point count increase/decrease failed
    // then consider it's not a key.
    // For example decreaseTouchPointCount() could fail if the key was autocommitted.
    if (!m_previousKey
        || !m_previousKey->enabled()
        || !m_previousKey->decreaseTouchPointCount()) {
        m_previousKey = 0;
    }
    if (!m_key
        || !m_key->enabled()
        || !m_key->increaseTouchPointCount()) {
        m_key = 0;
    }

    if (touchPointEnteredKey() && m_previousKey == 0) {
        keyHasGravity = true;
    } else if (touchPointLeftKey()) {
        keyHasGravity = false;
    }
}

bool MImAbstractKeyAreaPrivate::TouchPointRecord::touchPointEnteredKey() const
{
    return m_key && (m_previousKey != m_key);
}

bool MImAbstractKeyAreaPrivate::TouchPointRecord::touchPointLeftKey() const
{
    return m_previousKey && (m_previousKey != m_key);
}

bool MImAbstractKeyAreaPrivate::TouchPointRecord::hasGravity() const
{
    return m_key && keyHasGravity;
}

void MImAbstractKeyArea::setPanningEnabled(bool enable)
{
    Q_D(MImAbstractKeyArea);

    if (d->enabledPanning == enable) {
        return;
    }

    d->enabledPanning = enable;
    if (d->enabledPanning)
        grabGesture(BorderPanRecognizer::sharedGestureType());
    else
        ungrabGesture(BorderPanRecognizer::sharedGestureType());
}

bool MImAbstractKeyArea::panningEnabled() const
{
    Q_D(const MImAbstractKeyArea);
    return d->enabledPanning;
}

const QTouchEvent &MImAbstractKeyArea::lastTouchEvent() const
{
    Q_D(const MImAbstractKeyArea);
    return d->lastTouchEvent;
}
