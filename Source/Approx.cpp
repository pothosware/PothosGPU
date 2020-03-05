// Copyright (c) 2019-2020 Nicholas Corgan
// SPDX-License-Identifier: BSD-3-Clause

#include "OneToOneBlock.hpp"
#include "Utility.hpp"

#include <Pothos/Framework.hpp>
#include <Pothos/Object.hpp>

#include <arrayfire.h>

/*
 * As of 3.7.0, af::approx1 assumes the given interpolation
 * positions are evenly spaced from [0...len(in)].
 *
 * See: https://github.com/arrayfire/arrayfire/issues/2286
 */
static af::array mapPositionsToUniformInterval(
    const af::array& nodePositions,
    const af::array& desiredPositions)
{
    af::array ret = desiredPositions;
    //distances to normalize between two data points
    af::array dists = af::diff1(nodePositions);

    //get indices of two nearest data points to desired position ...
    //where / which index within the [nodePositions[0], nodePositions[1], nodePositions[2]] interval are the positions?
    af::array idxs = af::constant(0.0, 1, ret.dims(0), ::s32);
    gfor(af::seq i, ret.dims(0))
    {
        af::array temp = af::tile(ret(i), nodePositions.dims(0));
        idxs(0,i) = af::count(temp > nodePositions).as(::s32) - 1 ;
    }
    idxs(idxs < 0) = 0;
    idxs(idxs > (nodePositions.dims(0)-1)) = nodePositions.dims(0)-1;

    //find offset values
    af::array minvals = nodePositions(idxs);

    //subtract offsets, normalize, and move to uniform index locations
    ret -= minvals;
    ret /= dists(idxs);
    ret += idxs.T();
    return ret;
}
