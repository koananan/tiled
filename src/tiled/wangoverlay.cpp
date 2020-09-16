/*
 * wangoverlay.cpp
 * Copyright 2020, Thorbjørn Lindeijer <bjorn@lindeijer.nl>
 *
 * This file is part of Tiled.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "wangoverlay.h"

#include <QPainter>
#include <QPainterPath>

namespace Tiled {

static constexpr WangId oneCornerMask = WangId::MaskTopRight;
static constexpr WangId oneEdgeMask = WangId::MaskTop;
static constexpr WangId twoAdjacentCornersMask = WangId::MaskTopRight | WangId::MaskBottomRight;
static constexpr WangId twoOppositeCornersMask = WangId::MaskTopRight | WangId::MaskBottomLeft;
static constexpr WangId twoAdjacentEdgesMask = WangId::MaskTop | WangId::MaskRight;
static constexpr WangId twoOppositeEdgesMask = WangId::MaskTop | WangId::MaskBottom;
static constexpr WangId threeCornersMask = WangId::MaskTopRight | WangId::MaskBottomRight | WangId::MaskBottomLeft;
static constexpr WangId threeEdgesMask = WangId::MaskTop | WangId::MaskRight | WangId::MaskBottom;

struct PathWithMask {
    const QPainterPath path;
    const WangId mask;
};

static QPainterPath rotated(const QPainterPath &path, int rotations)
{
    QTransform transform;
    transform.translate(0.5, 0.5);
    transform.rotate(rotations * 90.0);
    transform.translate(-0.5, -0.5);
    return transform.map(path);
}

namespace EdgesAndCorners {

static const QPainterPath oneEdge = [] {
    constexpr qreal d = 1.0 / 6.0;
    QPainterPath path(QPointF(2 * d, 0));
    path.lineTo(4 * d, 0);
    path.lineTo(4 * d, 1 * d);
    path.arcTo(QRectF(QPointF(2 * d, 0), QSizeF(2 * d, 2 * d)), 0, -180);
    path.closeSubpath();
    return path;
}();

static const QPainterPath twoAdjacentEdges = [] {
    constexpr qreal d = 1.0 / 6.0;
    QPainterPath path(QPointF(2 * d, 0));
    path.lineTo(4 * d, 0);
    path.lineTo(4 * d, 1 * d);
    path.arcTo(QRectF(QPointF(4 * d, 0), QSizeF(2 * d, 2 * d)), -180, 90);
    path.lineTo(6 * d, 2 * d);
    path.lineTo(6 * d, 4 * d);
    path.lineTo(5 * d, 4 * d);
    path.arcTo(QRectF(QPointF(2 * d, -2 * d), QSizeF(6 * d, 6 * d)), -90, -90);
    path.closeSubpath();
    return path;
}();

static const QPainterPath twoOppositeEdges = [] {
    constexpr qreal d = 1.0 / 3.0;
    QPainterPath path;
    path.addRect(d, 0, d, d * 3);
    return path;
}();

static const QPainterPath threeEdges = [] {
    constexpr qreal d = 1.0 / 6.0;
    QPainterPath path(QPointF(2 * d, 0));
    path.lineTo(4 * d, 0);
    path.lineTo(4 * d, 1 * d);
    path.arcTo(QRectF(QPointF(4 * d, 0), QSizeF(2 * d, 2 * d)), -180, 90);
    path.lineTo(6 * d, 2 * d);
    path.lineTo(6 * d, 4 * d);
    path.lineTo(5 * d, 4 * d);
    path.arcTo(QRectF(QPointF(4 * d, 4 * d), QSizeF(2 * d, 2 * d)), 90, 90);
    path.lineTo(4 * d, 6 * d);
    path.lineTo(2 * d, 6 * d);
    path.closeSubpath();
    return path;
}();

static const QPainterPath fourEdges = [] {
    constexpr qreal d = 1.0 / 6.0;
    QPainterPath path(QPointF(2 * d, 0));
    path.lineTo(4 * d, 0);
    path.lineTo(4 * d, 1 * d);
    path.arcTo(QRectF(QPointF(4 * d, 0), QSizeF(2 * d, 2 * d)), -180, 90);
    path.lineTo(6 * d, 2 * d);
    path.lineTo(6 * d, 4 * d);
    path.lineTo(5 * d, 4 * d);
    path.arcTo(QRectF(QPointF(4 * d, 4 * d), QSizeF(2 * d, 2 * d)), 90, 90);
    path.lineTo(4 * d, 6 * d);
    path.lineTo(2 * d, 6 * d);
    path.arcTo(QRectF(QPointF(0, 4 * d), QSizeF(2 * d, 2 * d)), 0, 90);
    path.lineTo(0, 4 * d);
    path.lineTo(0, 2 * d);
    path.lineTo(d, 2 * d);
    path.arcTo(QRectF(QPointF(0, 0), QSizeF(2 * d, 2 * d)), -90, 90);
    path.closeSubpath();
    return path;
}();

static const QPainterPath *edgePathForMask(WangId mask)
{
    static const PathWithMask edgesWithMasks[] = {
        { fourEdges, WangId::MaskEdges },
        { threeEdges, threeEdgesMask },
        { rotated(threeEdges, 1), threeEdgesMask.rotated(1) },
        { rotated(threeEdges, 2), threeEdgesMask.rotated(2) },
        { rotated(threeEdges, 3), threeEdgesMask.rotated(3) },
        { twoAdjacentEdges, twoAdjacentEdgesMask },
        { rotated(twoAdjacentEdges, 1), twoAdjacentEdgesMask.rotated(1) },
        { rotated(twoAdjacentEdges, 2), twoAdjacentEdgesMask.rotated(2) },
        { rotated(twoAdjacentEdges, 3), twoAdjacentEdgesMask.rotated(3) },
        { twoOppositeEdges, twoOppositeEdgesMask },
        { rotated(twoOppositeEdges, 1), twoOppositeEdgesMask.rotated(1) },
        { oneEdge, oneEdgeMask },
        { rotated(oneEdge, 1), oneEdgeMask.rotated(1) },
        { rotated(oneEdge, 2), oneEdgeMask.rotated(2) },
        { rotated(oneEdge, 3), oneEdgeMask.rotated(3) },
    };

    for (auto &pathWithMask : edgesWithMasks)
        if (mask == pathWithMask.mask)
            return &pathWithMask.path;
    return nullptr;
}

static const QPainterPath oneCorner = [] {
    constexpr qreal d = 1.0 / 6.0;
    QPainterPath path(QPointF(4 * d, 0));
    path.lineTo(6 * d, 0);
    path.lineTo(6 * d, 2 * d);
    path.lineTo(5 * d, 2 * d);
    path.arcTo(QRectF(QPointF(4 * d, 0), QSizeF(2 * d, 2 * d)), -90, -90);
    path.closeSubpath();
    return path;
}();

static const QPainterPath twoAdjacentCorners = [] {
    QPainterPath path = oneCorner;
    path |= rotated(oneCorner, 1);
    return path;
}();

static const QPainterPath twoOppositeCorners = [] {
    QPainterPath path = oneCorner;
    path |= rotated(oneCorner, 2);
    return path;
}();

static const QPainterPath threeCorners = [] {
    QPainterPath path = twoAdjacentCorners;
    path |= rotated(oneCorner, 2);
    return path;
}();

static const QPainterPath fourCorners = [] {
    QPainterPath path = twoAdjacentCorners;
    path |= rotated(twoAdjacentCorners, 2);
    return path;
}();

static const QPainterPath *cornerPathForMask(WangId mask)
{
    static const PathWithMask cornersWithMasks[] = {
        { fourCorners, WangId::MaskCorners },
        { threeCorners, threeCornersMask },
        { rotated(threeCorners, 1), threeCornersMask.rotated(1) },
        { rotated(threeCorners, 2), threeCornersMask.rotated(2) },
        { rotated(threeCorners, 3), threeCornersMask.rotated(3) },
        { twoAdjacentCorners, twoAdjacentCornersMask },
        { rotated(twoAdjacentCorners, 1), twoAdjacentCornersMask.rotated(1) },
        { rotated(twoAdjacentCorners, 2), twoAdjacentCornersMask.rotated(2) },
        { rotated(twoAdjacentCorners, 3), twoAdjacentCornersMask.rotated(3) },
        { twoOppositeCorners, twoOppositeCornersMask },
        { rotated(twoOppositeCorners, 1), twoOppositeCornersMask.rotated(1) },
        { oneCorner, oneCornerMask },
        { rotated(oneCorner, 1), oneCornerMask.rotated(1) },
        { rotated(oneCorner, 2), oneCornerMask.rotated(2) },
        { rotated(oneCorner, 3), oneCornerMask.rotated(3) },
    };

    for (auto &pathWithMask : cornersWithMasks)
        if (mask == pathWithMask.mask)
            return &pathWithMask.path;
    return nullptr;
}

} // namespace EdgesAndCorners

namespace EdgesOnly {

static const QPainterPath oneEdge = [] {
    constexpr qreal d = 1.0 / 6.0;
    QPainterPath path(QPointF(5 * d, 0));
    path.arcTo(QRectF(QPointF(d, -2 * d), QSizeF(4 * d, 4 * d)), 0, -180);
    path.closeSubpath();
    return path;
}();

static const QPainterPath twoAdjacentEdges = [] {
    constexpr qreal d = 1.0 / 6.0;
    QPainterPath path(QPointF(5 * d, 0));
    path.arcTo(QRectF(QPointF(5 * d, -d), QSizeF(2 * d, 2 * d)), 180, 90);
    path.lineTo(6 * d, 5 * d);
    path.arcTo(QRectF(QPointF(d, -5 * d), QSizeF(10 * d, 10 * d)), -90, -90);
    path.closeSubpath();
    return path;
}();

static const QPainterPath twoOppositeEdges = [] {
    constexpr qreal d = 1.0 / 6.0;
    QPainterPath path;
    path.addRect(d, 0, 4 * d, d * 6);
    return path;
}();

static const QPainterPath threeEdges = [] {
    constexpr qreal d = 1.0 / 6.0;
    QPainterPath path(QPointF(5 * d, 0));
    path.arcTo(QRectF(QPointF(5 * d, -d), QSizeF(2 * d, 2 * d)), 180, 90);
    path.lineTo(6 * d, 5 * d);
    path.arcTo(QRectF(QPointF(5 * d, 5 * d), QSizeF(2 * d, 2 * d)), 90, 90);
    path.lineTo(d, 6 * d);
    path.lineTo(d, 0);
    path.closeSubpath();
    return path;
}();

static const QPainterPath fourEdges = [] {
    constexpr qreal d = 1.0 / 6.0;
    QPainterPath path(QPointF(5 * d, 0));
    path.arcTo(QRectF(QPointF(5 * d, -d), QSizeF(2 * d, 2 * d)), 180, 90);
    path.lineTo(6 * d, 5 * d);
    path.arcTo(QRectF(QPointF(5 * d, 5 * d), QSizeF(2 * d, 2 * d)), 90, 90);
    path.lineTo(d, 6 * d);
    path.arcTo(QRectF(QPointF(-d, 5 * d), QSizeF(2 * d, 2 * d)), 0, 90);
    path.lineTo(0, d);
    path.arcTo(QRectF(QPointF(-d, -d), QSizeF(2 * d, 2 * d)), -90, 90);
    path.closeSubpath();
    return path;
}();

static const QPainterPath *pathForMask(WangId mask)
{
    static const PathWithMask edgesWithMasks[] = {
        { fourEdges, WangId::MaskEdges },
        { threeEdges, threeEdgesMask },
        { rotated(threeEdges, 1), threeEdgesMask.rotated(1) },
        { rotated(threeEdges, 2), threeEdgesMask.rotated(2) },
        { rotated(threeEdges, 3), threeEdgesMask.rotated(3) },
        { twoAdjacentEdges, twoAdjacentEdgesMask },
        { rotated(twoAdjacentEdges, 1), twoAdjacentEdgesMask.rotated(1) },
        { rotated(twoAdjacentEdges, 2), twoAdjacentEdgesMask.rotated(2) },
        { rotated(twoAdjacentEdges, 3), twoAdjacentEdgesMask.rotated(3) },
        { twoOppositeEdges, twoOppositeEdgesMask },
        { rotated(twoOppositeEdges, 1), twoOppositeEdgesMask.rotated(1) },
        { oneEdge, oneEdgeMask },
        { rotated(oneEdge, 1), oneEdgeMask.rotated(1) },
        { rotated(oneEdge, 2), oneEdgeMask.rotated(2) },
        { rotated(oneEdge, 3), oneEdgeMask.rotated(3) },
    };

    for (auto &pathWithMask : edgesWithMasks)
        if (mask == pathWithMask.mask)
            return &pathWithMask.path;
    return nullptr;
}

} // namespace EdgesOnly

namespace CornersOnly {

static const QPainterPath oneCorner = [] {
    QPainterPath path(QPointF(0.5, 0));
    path.arcTo(QRectF(QPointF(0.5, -0.5), QSizeF(1, 1)), 180, 90);
    path.lineTo(1, 0);
    path.closeSubpath();
    return path;
}();

static const QPainterPath twoAdjacentCorners = [] {
    QPainterPath path;
    path.addRect(0.5, 0, 0.5, 1);
    return path;
}();

static const QPainterPath twoOppositeCorners = [] {
    QPainterPath path = oneCorner;
    path |= rotated(oneCorner, 2);
    return path;
}();

static const QPainterPath threeCorners = [] {
    QPainterPath path(QPointF(1, 0));
    path.lineTo(1, 1);
    path.lineTo(0, 1);
    path.lineTo(0, 0.5);
    path.arcTo(QRectF(QPointF(-0.5, -0.5), QSizeF(1, 1)), -90, 90);
    path.closeSubpath();
    return path;
}();

static const QPainterPath fourCorners = [] {
    QPainterPath path;
    path.addRect(0, 0, 1, 1);
    return path;
}();

static const QPainterPath *pathForMask(WangId mask)
{
    static const PathWithMask cornersWithMasks[] = {
        { fourCorners, WangId::MaskCorners },
        { threeCorners, threeCornersMask },
        { rotated(threeCorners, 1), threeCornersMask.rotated(1) },
        { rotated(threeCorners, 2), threeCornersMask.rotated(2) },
        { rotated(threeCorners, 3), threeCornersMask.rotated(3) },
        { twoAdjacentCorners, twoAdjacentCornersMask },
        { rotated(twoAdjacentCorners, 1), twoAdjacentCornersMask.rotated(1) },
        { rotated(twoAdjacentCorners, 2), twoAdjacentCornersMask.rotated(2) },
        { rotated(twoAdjacentCorners, 3), twoAdjacentCornersMask.rotated(3) },
        { twoOppositeCorners, twoOppositeCornersMask },
        { rotated(twoOppositeCorners, 1), twoOppositeCornersMask.rotated(1) },
        { oneCorner, oneCornerMask },
        { rotated(oneCorner, 1), oneCornerMask.rotated(1) },
        { rotated(oneCorner, 2), oneCornerMask.rotated(2) },
        { rotated(oneCorner, 3), oneCornerMask.rotated(3) },
    };

    for (auto &pathWithMask : cornersWithMasks)
        if (mask == pathWithMask.mask)
            return &pathWithMask.path;
    return nullptr;
}

} // namespace CornersOnly

static void setCosmeticPen(QPainter *painter, const QBrush &brush, qreal width)
{
    const qreal devicePixelRatio = painter->device()->devicePixelRatioF();
    QPen pen(brush, width * devicePixelRatio);
    pen.setCosmetic(true);
    painter->setPen(pen);
}

void paintWangOverlay(QPainter *painter,
                      WangId wangId,
                      const WangSet &wangSet,
                      const QRect &rect,
                      bool transparent)
{
    if (!wangId)
        return;

    const QRect adjustedRect = rect.adjusted(2, 2, -2, -2);
    if (adjustedRect.isEmpty())
        return;

    const qreal fillOpacity = transparent ? 0.3 : 1.0;
    const qreal penWidth = qMin(2.0, adjustedRect.width() / 16.0);

    painter->save();
    painter->setClipRect(rect);
    painter->setRenderHint(QPainter::Antialiasing);

    QTransform foregroundTransform = painter->transform();
    foregroundTransform.translate(adjustedRect.left(), adjustedRect.top());

    QTransform shadowTransform = foregroundTransform;
    shadowTransform.translate(1, 1);

    shadowTransform.scale(adjustedRect.width(), adjustedRect.height());
    foregroundTransform.scale(adjustedRect.width(), adjustedRect.height());

    for (int color = 1; color <= wangSet.colorCount(); ++color) {
        const WangId mask = wangId.mask(color);
        if (!mask)
            continue;

        const QPainterPath *cornerPath = nullptr;
        const QPainterPath *edgePath = nullptr;

        switch (wangSet.type()) {
        case WangSet::Corner:
        case WangSet::Edge:
            // One of these should be nullptr, but if it isn't we may want to
            // see that the Wang set is a little messed up.
            cornerPath = CornersOnly::pathForMask(mask & WangId::MaskCorners);
            edgePath = EdgesOnly::pathForMask(mask & WangId::MaskEdges);
            break;
        case WangSet::Mixed:
            cornerPath = EdgesAndCorners::cornerPathForMask(mask & WangId::MaskCorners);
            edgePath = EdgesAndCorners::edgePathForMask(mask & WangId::MaskEdges);
            break;
        }

        // Draw the shadow
        if (transparent) {
            painter->setBrush(Qt::NoBrush);
            setCosmeticPen(painter, Qt::black, penWidth);
            painter->setTransform(shadowTransform);

            if (cornerPath)
                painter->drawPath(*cornerPath);
            if (edgePath)
                painter->drawPath(*edgePath);
        }

        // Draw the foreground
        const QColor c = wangSet.colorAt(color)->color();
        painter->setBrush(QColor(c.red(), c.green(), c.blue(), c.alpha() * fillOpacity));

        if (transparent)
            setCosmeticPen(painter, c, penWidth);
        else
            setCosmeticPen(painter, Qt::black, penWidth);

        painter->setTransform(foregroundTransform);

        if (cornerPath)
            painter->drawPath(*cornerPath);
        if (edgePath)
            painter->drawPath(*edgePath);
    }

    painter->restore();
}

} // namespace Tiled
