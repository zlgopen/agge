//----------------------------------------------------------------------------
// Anti-Grain Geometry (AGG) - Version 2.5
// A high quality rendering engine for C++
// Copyright (C) 2002-2006 Maxim Shemanarev
// Contact: mcseem@antigrain.com
//          mcseemagg@yahoo.com
//          http://antigrain.com
// 
// AGG is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// AGG is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with AGG; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, 
// MA 02110-1301, USA.
//----------------------------------------------------------------------------

#include <math.h>
#include "aggx_vcgen_stroke.h"
#include "aggx_shorten_path.h"

namespace aggx
{
	//------------------------------------------------------------------------
vcgen_stroke::vcgen_stroke() :
	m_stroker(),
	m_shorten(0.0),
	m_closed(0),
	m_status(initial),
	m_src_vertex(0),
	m_out_vertex(0)
{
}

//------------------------------------------------------------------------
void vcgen_stroke::remove_all()
{
	m_src_vertices.clear();
	m_closed = 0;
	m_status = initial;
}


//------------------------------------------------------------------------
void vcgen_stroke::add_vertex(real x, real y, unsigned cmd)
{
	m_status = initial;
	if(is_move_to(cmd))
	{
		m_src_vertices.modify_last(vertex_dist(x, y));
	}
	else
	{
		if(is_vertex(cmd))
		{
			m_src_vertices.add(vertex_dist(x, y));
		}
		else
		{
			m_closed = get_close_flag(cmd);
		}
	}
}

//------------------------------------------------------------------------
void vcgen_stroke::rewind(unsigned)
{
	if(m_status == initial)
	{
		m_src_vertices.close(m_closed != 0);
		shorten_path(m_src_vertices, m_shorten, m_closed);
		if(m_src_vertices.size() < 3) m_closed = 0;
	}
	m_status = ready;
	m_src_vertex = 0;
	m_out_vertex = 0;
}


//------------------------------------------------------------------------
unsigned vcgen_stroke::vertex(real* x, real* y)
{
	unsigned cmd = path_cmd_line_to;
	while(!is_stop(cmd))
	{
		switch(m_status)
		{
		case initial:
			rewind(0);

		case ready:
			if(m_src_vertices.size() < 2 + unsigned(m_closed != 0))
			{
				cmd = path_cmd_stop;
				break;
			}
			m_status = m_closed ? outline1 : cap1;
			cmd = path_cmd_move_to;
			m_src_vertex = 0;
			m_out_vertex = 0;
			break;

		case cap1:
			m_stroker.calc_cap(m_out_vertices,
				m_src_vertices[0], 
				m_src_vertices[1], 
				m_src_vertices[0].dist);
			m_src_vertex = 1;
			m_prev_status = outline1;
			m_status = out_vertices;
			m_out_vertex = 0;
			break;

		case cap2:
			m_stroker.calc_cap(m_out_vertices,
				m_src_vertices[m_src_vertices.size() - 1], 
				m_src_vertices[m_src_vertices.size() - 2], 
				m_src_vertices[m_src_vertices.size() - 2].dist);
			m_prev_status = outline2;
			m_status = out_vertices;
			m_out_vertex = 0;
			break;

		case outline1:
			if(m_closed)
			{
				if(m_src_vertex >= m_src_vertices.size())
				{
					m_prev_status = close_first;
					m_status = end_poly1;
					break;
				}
			}
			else
			{
				if(m_src_vertex >= m_src_vertices.size() - 1)
				{
					m_status = cap2;
					break;
				}
			}
			m_stroker.calc_join(m_out_vertices, 
				m_src_vertices.prev(m_src_vertex), 
				m_src_vertices.curr(m_src_vertex), 
				m_src_vertices.next(m_src_vertex), 
				m_src_vertices.prev(m_src_vertex).dist,
				m_src_vertices.curr(m_src_vertex).dist);
			++m_src_vertex;
			m_prev_status = m_status;
			m_status = out_vertices;
			m_out_vertex = 0;
			break;

		case close_first:
			m_status = outline2;
			cmd = path_cmd_move_to;

		case outline2:
			if(m_src_vertex <= unsigned(m_closed == 0))
			{
				m_status = end_poly2;
				m_prev_status = stop;
				break;
			}

			--m_src_vertex;
			m_stroker.calc_join(m_out_vertices,
				m_src_vertices.next(m_src_vertex), 
				m_src_vertices.curr(m_src_vertex), 
				m_src_vertices.prev(m_src_vertex), 
				m_src_vertices.curr(m_src_vertex).dist, 
				m_src_vertices.prev(m_src_vertex).dist);

			m_prev_status = m_status;
			m_status = out_vertices;
			m_out_vertex = 0;
			break;

		case out_vertices:
			if(m_out_vertex >= m_out_vertices.size())
			{
				m_status = m_prev_status;
			}
			else
			{
				const point_r& c = m_out_vertices[m_out_vertex++];
				*x = c.x;
				*y = c.y;
				return cmd;
			}
			break;

		case end_poly1:
			m_status = m_prev_status;
			return path_cmd_end_poly | path_flags_close | path_flags_ccw;

		case end_poly2:
			m_status = m_prev_status;
			return path_cmd_end_poly | path_flags_close | path_flags_cw;

		case stop:
			cmd = path_cmd_stop;
			break;
		}
	}
	return cmd;
}

}