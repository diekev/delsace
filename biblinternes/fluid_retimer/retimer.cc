/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software  Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#if 0

#include "retimer.h"

#include <openvdb/tools/Composite.h>
#include <openvdb/tools/Interpolation.h>
#include <openvdb/tools/PointAdvect.h> // for velocity integrator

using namespace openvdb;

class RetimingOp {
	using VelIntegrator = openvdb::tools::VelocityIntegrator<VectorGrid>;
	using FloatLeaf = FloatTree::LeafNodeType;
	using SamplerF = tools::GridSampler<FloatGrid::ConstAccessor, tools::BoxSampler>;

	FloatGrid::Ptr m_grid;
	FloatGrid::Accessor m_accessor;
	FloatGrid::ConstAccessor m_accessor_t0, m_accessor_t1;
	VelIntegrator m_vel_field_t0, m_vel_field_t1;

	int m_fwd_steps;
	int m_bck_steps;
	float m_time_scale;
	float m_blur_weights[3];
	float m_fwd_dt[3], m_bwd_dt[3];
	float m_scale;

public:
	RetimingOp(ScalarGrid::Ptr work,
			 ScalarGrid::Ptr gridA,
			 ScalarGrid::Ptr gridB,
			 VectorGrid::Ptr velA,
			 VectorGrid::Ptr velB,
			 int steps, float time_scale, float shutter_speed)
		: m_grid(work)
		, m_accessor(work->getAccessor())
		, m_accessor_t0(gridA->getConstAccessor())
		, m_accessor_t1(gridB->getConstAccessor())
		, m_vel_field_t0(*velA)
		, m_vel_field_t1(*velB)
		, m_fwd_steps(steps)
		, m_bck_steps(MAX_STEPS - steps)
		, m_time_scale(time_scale)
		, m_scale(1.0f / 3.0f)
	{
		/* To counter noise produced by the advection steps we simulate a motion
		 * blur type of effect. In order to avoid more funky code we hardcode the
		 * number of blurring steps to three and use the following constants as
		 * weights.
		 */
		m_blur_weights[0] = time_scale - 0.5f * shutter_speed;
		m_blur_weights[1] = time_scale;
		m_blur_weights[2] = time_scale + 0.5f * shutter_speed;

		/* Precompute forward and backward steps */
		for (int i = 0; i < 3; ++i) {
			m_fwd_dt[i] = (1.0f / m_fwd_steps) * m_blur_weights[i];
			m_bwd_dt[i] = (1.0f / m_bck_steps) * (1.0f - m_blur_weights[i]);
		}
	}

	RetimingOp(RetimingOp &other, tbb::split)
		: m_grid(other.m_grid->deepCopy())
		, m_accessor(m_grid->getAccessor())
		, m_accessor_t0(other.m_accessor_t0)
		, m_accessor_t1(other.m_accessor_t1)
		, m_vel_field_t0(other.m_vel_field_t0)
		, m_vel_field_t1(other.m_vel_field_t1)
		, m_fwd_steps(other.m_fwd_steps)
		, m_bck_steps(other.m_bck_steps)
		, m_time_scale(other.m_time_scale)
		, m_scale(other.m_scale)
	{
		for (int i = 0; i < 3; ++i) {
			m_blur_weights[i] = other.m_blur_weights[i];
			m_fwd_dt[i] = other.m_fwd_dt[i];
			m_bwd_dt[i] = other.m_bwd_dt[i];
		}
	}

	~RetimingOp()
	{}

	using IterRange = tree::IteratorRange<FloatTree::LeafCIter>;

	void operator()(IterRange &range)
	{
		SamplerF sampler_t0(m_accessor_t0, m_grid->transform());
		SamplerF sampler_t1(m_accessor_t1, m_grid->transform());

		for (; range; ++range) {
			for (FloatLeaf::ValueOnCIter iter(range.iterator()->cbeginValueOn()); iter; ++iter) {
				float val_t0 = 0.0f, val_t1 = 0.0f;
				const auto coord = iter.getCoord();

				for (int i = 0; i < 3; ++i) {
					/* forward advection from t to t+n */
					auto coord_t0 = coord.asVec3i();

					for (int j = 0; j < m_fwd_steps; ++j) {
						m_vel_field_t0.rungeKutta<1>(m_fwd_dt[i], coord_t0);
					}
					val_t0 += sampler_t0.isSample(coord_t0);

					/* backward advection from t+1 to t+n */
					auto coord_t1 = coord.asVec3i();

					for (int j = 0; j < m_bck_steps; ++j) {
						m_vel_field_t1.rungeKutta<1>(-m_bwd_dt[i], coord_t1);
					}
					val_t1 += sampler_t1.isSample(coord_t1);
				}

				val_t0 = val_t0 * m_scale;
				val_t1 = val_t1 * m_scale;

				m_accessor.setValue(coord, lerp(val_t0, val_t1, m_time_scale));
			}
		}
	}

	void join(RetimingOp &other)
	{
		openvdb::tools::compSum(*m_grid, *other.m_grid);
	}
};

Retimer::Retimer()
{
	m_num_steps = 0;
	m_time_scale = 1.0f;
	m_shutter_speed = 0.0f;
	m_threaded = false;
}

Retimer::Retimer(int steps, float time_scale, float shutter_speed)
	: m_num_steps(steps)
	, m_time_scale(time_scale)
	, m_shutter_speed(shutter_speed)
	, m_threaded(false)
{}

Retimer::~Retimer()
{
	m_grids.clear();
	m_grid_names.clear();
}

void Retimer::setTimeScale(const float scale)
{
	m_time_scale = scale;
}

void Retimer::setThreaded(const bool threaded)
{
	m_threaded = threaded;
}

/**
 * Sets the name for the grids to retime.
 *
 * @param list a list of grid name.
 */
void Retimer::setGridNames(std::initializer_list<std::string> list)
{
	for (const auto item : list) {
		m_grid_names.push_back(item);
	}
}

/**
 * This is the function to be called by the client code. It loads the
 * corresponding grids from the current and next frame. The actual retiming is
 * done in readvectSL().
 *
 * @param previous the file containing the grids for the previous frame
 * @param cur the file containing the grids for the current frame
 * @param to the file where to put the retimed grids
 */
void Retimer::retime(const std::string &previous, const std::string &cur, const std::string &to)
{
	openvdb::io::File file_p(previous);
	file_p.open();
	openvdb::io::File file_c(cur);
	file_c.open();

	openvdb::FloatGrid::Ptr gridA, gridB, retimed;
	VectorGrid::Ptr velGridA = openvdb::gridPtrCast<VectorGrid>(file_p.readGrid("velocity"));
	VectorGrid::Ptr velGridB = openvdb::gridPtrCast<VectorGrid>(file_c.readGrid("velocity"));

	for (auto i = 0ul; i < m_grid_names.size(); ++i) {
		openvdb::GridBase::Ptr gridA_tmp = file_p.readGrid(m_grid_names[i]);
		openvdb::GridBase::Ptr gridB_tmp = file_c.readGrid(m_grid_names[i]);

		gridA = openvdb::gridPtrCast<openvdb::FloatGrid>(gridA_tmp);
		gridB = openvdb::gridPtrCast<openvdb::FloatGrid>(gridB_tmp);

		retimed = openvdb::FloatGrid::create(gridA->background());
		retimed->setTransform(gridA->transform().copy());
		retimed->setName(m_grid_names[i]);

		readvectSL(retimed, gridA, gridB, velGridA, velGridB);

		m_grids.push_back(retimed);
	}

	openvdb::io::File file_to(to);
	file_to.write(m_grids);
	file_to.close();
	file_p.close();
	file_c.close();
	m_grids.clear();
}

/**
 * This does the actual retiming process. We basically do a forward and backward
 * readvection of the fluid field.
 *
 * @param work the retimed grid
 * @param gridA the grid for the current frame
 * @param gridB the grid for the next frame
 * @param velA the velocity grid for the current frame
 * @param velB the velocity grid for the next frame
 */
void Retimer::readvectSL(ScalarGrid::Ptr work,
						 ScalarGrid::Ptr gridA,
						 ScalarGrid::Ptr gridB,
						 VectorGrid::Ptr velA,
						 VectorGrid::Ptr velB)
{
	RetimingOp op(
			work, gridA, gridB, velA, velB,
			m_num_steps, m_time_scale, m_shutter_speed);

	RetimingOp::IterRange range(gridA->tree().cbeginLeaf());

	if (m_threaded) {
		tbb::parallel_reduce(range, op);
	}
	else {
		op(range);
	}
}

#if 0
float step_size = 0.1f;
int n_steps = 1.0f / step_size; // 10 steps
int fwd_steps = time_scale / step_size; // time_scale = 0.3 -> 3 steps.
int bwd_steps = n_steps - fwd_steps;

float readvectSL(const int steps, const float *vel_x, float *field, const float *vel_y, const float *vel_z, const float pos[3], const float dt, int res[3])
{
	float pos0[3] = { pos[0], pos[1], pos[2] };

	for (int j = 0; j < steps; ++j) {
		float vel_x0 = sample(vel_x, pos0, res);
		float vel_y0 = sample(vel_y, pos0, res);
		float vel_z0 = sample(vel_z, pos0, res);
		pos0[0] = pos0[0] - dt * vel_x0;
		pos0[1] = pos0[1] - dt * vel_y0;
		pos0[2] = pos0[2] - dt * vel_z0;
	}

	return sample(field, pos0, res);
}

void readvect_fluid(const int num_steps, const float time_scale, const float shutter_speed)
{
	int res[3] = {0};
	float *field = nullptr, *field_next = nullptr, *dens_retimed = nullptr;
	float *vel_x = nullptr, *vel_x_next = nullptr;
	float *vel_y = nullptr, *vel_y_next = nullptr;
	float *vel_z = nullptr, *vel_z_next = nullptr;

	/* To counter noise produced by the advection steps we simulate a motion
	 * blur type of effect. In order to avoid more funky code we hardcode the
	 * number of blurring steps to three and use the following constants as
	 * weights.
	 */
	const float ntime_scale[3] = {
		time_scale - 0.5f * shutter_speed,
		time_scale,
		time_scale + 0.5f * shutter_speed
	};

	const float scale = 1.0f / 3.0f;

	for (int z = 0; z < res[2]; ++z) {
		for (int y = 0; y < res[1]; ++y) {
			for (int x = 0; x < res[0]; ++x) {
				/* 0 = current frame, 1 = next frame. */
				float dens0 = 0.0f, dens1 = 0.0f;

				for (int i = 0; i < 3; ++i) {

					/* forward advection from t to t+n */
					float pos0[3] = { x, y, z };
					float dt = (1.0f / num_steps) * ntime_scale[i];

					for (int j = 0; j < num_steps; ++j) {
						float vel_x0 = sample(vel_x, pos0, res);
						float vel_y0 = sample(vel_y, pos0, res);
						float vel_z0 = sample(vel_z, pos0, res);
						pos0[0] = pos0[0] - dt * vel_x0;
						pos0[1] = pos0[1] - dt * vel_y0;
						pos0[2] = pos0[2] - dt * vel_z0;
					}

					dens0 += sample(field, pos0, res);

					/* backward advection from t+1 to t+n */
					const int backward_steps = MAX_STEPS - num_steps;
					dt = (1.0f / backward_steps) * (1.0f - ntime_scale[i]);
					float pos1[3] = { x, y, z };

					for (int j = 0; j < backward_steps; ++j) {
						float vel_x1 = sample(vel_x_next, pos1, res);
						float vel_y1 = sample(vel_y_next, pos1, res);
						float vel_z1 = sample(vel_z_next, pos1, res);
						pos1[0] = pos1[0] + dt * vel_x1;
						pos1[1] = pos1[1] + dt * vel_y1;
						pos1[2] = pos1[2] + dt * vel_z1;
					}

					dens1 += sample(field_next, pos1, res);
				}

				dens0 = dens0 * scale;
				dens1 = dens1 * scale;

				/* new density */
				int index = x + y * res[0] + z * res[0] * res[1];
				dens_retimed[index] = lerp(dens0, dens1, time_scale);
			}
		}
	}
}
#endif

#endif
