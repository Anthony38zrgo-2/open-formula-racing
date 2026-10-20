/***************************************************************************

    file        : LearnedGraph.cpp
    created     : 9 Apr 2006
    copyright   : (C) 2006 Tim Foden

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "LearnedGraph.h"
#include <math.h>
#include <vector>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

LearnedGraph::LearnedGraph()
:	m_beta(0.5)
{
}

LearnedGraph::LearnedGraph(
    int				nAxes,
    const double*	min,
    const double*	max,
    const int*		steps,
    double			initialValue )
:
    LearnedGraph()
{
    m_pAxis.reserve(nAxes);
    int	itemSize = 1;
    for( int i = nAxes - 1; i >= 0; i-- )
    {
        Axis axis;

        axis.m_min = min[i];
        axis.m_span = max[i] - min[i];
        axis.m_steps = steps[i];
        axis.m_itemSize = itemSize;
        m_pAxis.push_back(axis);

        itemSize *= steps[i] + 1;
    }

    m_pData.reserve(itemSize);
    for( int i = 0; i < itemSize; i++ )
        m_pData.push_back(initialValue);
}

LearnedGraph::LearnedGraph( double minX, double maxX, int xSteps, double initialY )
:	LearnedGraph()
{
    Axis axis;

    axis.m_min = minX;
    axis.m_span = maxX - minX;
    axis.m_steps = xSteps;
    axis.m_itemSize = 1;
    m_pAxis.push_back(axis);

    m_pData.reserve(xSteps + 1);
    for( int i = 0; i <= xSteps; i++ )
        m_pData.push_back(initialY);
}

LearnedGraph::axis_size		LearnedGraph::GetNAxes() const
{
    return m_pAxis.size();
}

void	LearnedGraph::Learn( double x, double value )
{
    std::vector<double> coord;

    coord.push_back(x);
    Learn( coord, value );
}

void	LearnedGraph::Learn( const std::vector<double> &coord, double value )
{
    std::vector<Idx> idx;
    MakeIdx(coord, idx);

    double	oldValue = CalcValue(0, 0, idx);
    double	delta = m_beta * (value - oldValue);
    LearnValue( 0, 0, idx, delta );
}

double	LearnedGraph::CalcY( double x ) const
{
    return CalcValue(x);
}

double	LearnedGraph::CalcValue( double coord ) const
{
    std::vector<Idx> idx;

    idx.push_back(MakeIdx(m_pAxis[0], coord));
    return CalcValue(0, 0, idx);
}

double	LearnedGraph::CalcValue( const std::vector<double> &coord ) const
{
    std::vector<Idx> idx;
    MakeIdx(coord, idx);
    double	value = CalcValue(0, 0, idx);
    return value;
}

double	LearnedGraph::GetY( int index ) const
{
    return m_pData[index];
}

void	LearnedGraph::SetBeta( double beta )
{
    m_beta = beta;
}

double	LearnedGraph::CalcValue( axis_size dim, int offs, const std::vector<Idx> &idx ) const
{
    if( dim < m_pAxis.size() )
    {
        int		offs_i = offs + m_pAxis[dim].m_itemSize * idx[dim].i;
        int		offs_j = offs + m_pAxis[dim].m_itemSize * idx[dim].j;

        double	a = CalcValue(dim + 1, offs_i, idx);
        double	b = CalcValue(dim + 1, offs_j, idx);

        return a * (1 - idx[dim].t) + b * idx[dim].t;
    }
    else
        return m_pData[offs];
}

void	LearnedGraph::LearnValue( axis_size dim, int offs, const std::vector<Idx> &idx, double delta )
{
    if( dim < m_pAxis.size() )
    {
        int		offs_i = offs + m_pAxis[dim].m_itemSize * idx[dim].i;
        int		offs_j = offs + m_pAxis[dim].m_itemSize * idx[dim].j;

        LearnValue( dim + 1, offs_i, idx, delta * (1 - idx[dim].t) );
        LearnValue( dim + 1, offs_j, idx, delta * idx[dim].t );
    }
    else
        m_pData[offs] += delta;
}

LearnedGraph::Idx    LearnedGraph::MakeIdx( const Axis &src, double coord ) const
{
    Idx idx;

    // 0 <= t <= m_steps
    idx.t = src.m_steps * (coord - src.m_min) / src.m_span;
    if( idx.t < 0 )
        idx.t = 0;
    else if( idx.t > src.m_steps )
        idx.t = src.m_steps;

    idx.i = floor(idx.t);
    idx.j = idx.i < src.m_steps ? idx.i + 1 : src.m_steps;
    idx.t = idx.t - idx.i;	// 0 <= t <= 1

    return idx;
}

void	LearnedGraph::MakeIdx( const std::vector<double> &coord, std::vector<Idx> &idx ) const
{
    idx.reserve(m_pAxis.size());

    for( axis_size i = 0; i < m_pAxis.size(); i++ )
        idx.push_back(MakeIdx(m_pAxis[i], coord[i]));
}
