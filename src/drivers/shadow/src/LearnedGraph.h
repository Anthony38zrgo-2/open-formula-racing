/***************************************************************************

    file        : LearnedGraph.h
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

#ifndef _LEARNEDGRAPH_H_
#define _LEARNEDGRAPH_H_

#include <vector>

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class LearnedGraph
{
public:
    LearnedGraph();
    LearnedGraph( int nAxes, const double* minX, const double* maxX,
                    const int* xSteps, double initialValue  );
    LearnedGraph( double minX, double maxX, int xSteps, double initialY );
    typedef std::vector<struct Axis>::size_type axis_size;

    axis_size		GetNAxes() const;

    void	Learn( double x, double value );
    void	Learn( const std::vector<double> &coord, double value );

    double	CalcY( double x ) const;
    double	CalcValue( const std::vector<double> &coord ) const;

    double	GetY( int index ) const;

    void	SetBeta( double beta );

private:
    struct Axis
    {
        double	m_min;
        double	m_span;
        int		m_steps;
        int		m_itemSize;
    };

    struct Idx
    {
        int		i;
        int		j;
        double	t;
    };

private:
    double	CalcValue( axis_size dim, int offs, const std::vector<Idx> &idx ) const;
    double	CalcValue( double coord ) const;
    void	LearnValue( axis_size dim, int offs, const std::vector<Idx> &idx, double delta );
    void	MakeIdx( const std::vector<double> &coord, std::vector<Idx> &out ) const;
    Idx    MakeIdx( const Axis &src, double coord ) const;

private:
    double	m_beta;
    std::vector<Axis>	m_pAxis;
    std::vector<double>	m_pData;
};

#endif // _LEARNEDGRAPH_H_
