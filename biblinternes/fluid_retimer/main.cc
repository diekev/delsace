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

#include <sys/time.h>

#include "../../biblexternes/docopt/docopt.hh"

#include "retimer.h"

static const char usage[] = R"(
Fluid Retimer.

Usage:
	retimer FILE --frame_start=<fs> --frame_end=<fe> --dt=<kn> --steps=<x> --shutter=<y> [--threaded]
	retimer (-h | --help)
	retimer --version

Options:
	-h, --help    Show this screen.
	--version     Show version.
	--frame_start Start frame number.
	--frame_end   End frame number.
	--dt          Time scale used by the retimer.
	--steps       Number of forward steps.
	--shutter     Shutter speed used for denoising the retimed process.
	--threaded    Use multi-threading.
)";

double time_dt()
{
	struct timeval now;
	gettimeofday(&now, nullptr);

	return now.tv_sec + now.tv_usec*1e-6;
}

dls::chaine get_name_for_frame(const dls::chaine &basename, const int frame)
{
	std::ostringstream os;
	os << frame;
	dls::chaine framenr = os.str();
	framenr.insere(framenr.debut(), 4 - framenr.taille(), '0');

	return basename + framenr + ".vdb";
}

dls::tableau<dls::chaine> generate_filenames(const dls::chaine &name, const int num_frame)
{
	dls::tableau<dls::chaine> v;

	for (int i = 0; i < num_frame; ++i) {
		v.pousse(get_name_for_frame(name, i + 1));
//		std::cout << v[i] << std::endl;
	}

	return v;
}

void copyFile(const dls::chaine &from_file, const dls::chaine &to_file)
{
	openvdb::io::File from(from_file), to(to_file);
	from.open();
	to.write(*from.getGrids());
	to.close();
	from.close();
}

void catch_exception()
{
	try {
		throw;
	}
	catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
	}
	catch (...) {
		std::cerr << "Unknown error in OpenVDB library..." << std::endl;
	}
}

int main(int argc, const char *argv[])
{
	auto args = dls::docopt::docopt(usage, { argv + 1, argv + argc }, true, "Retimer 0.1");

	const auto filename = args["FILE"].asString();
	const auto time_scale = std::stof(args["--dt"].asString());
	const auto num_steps = std::stoi(args["--steps"].asString());
	const auto shutter_speed = std::stof(args["--shutter"].asString());
	const auto threaded = args["--threaded"].asBool();
	const auto start_frame = std::stoi(args["--frame_start"].asString());
	const auto end_frame = std::stoi(args["--frame_end"].asString());

	openvdb::initialize();

	const auto frame_step = 1.0f / time_scale;
	const auto time_step = time_scale * 100;
	const auto basename = filename.substr(0, filename.taille() - 8);
	const auto frame_range = end_frame - start_frame + 1;
	const auto files = generate_filenames(basename, frame_range);
	const auto r_files = generate_filenames(basename + "retime.", frame_range * frame_step);

	auto frame(0);
	auto time_step_t(0);

	std::cout << "Time scale: " << time_scale << std::endl;

	double time_start = time_dt();

	Retimer retimer(num_steps, time_scale, shutter_speed);
	retimer.setGridNames({"density", "heat", "heat_old"});
	retimer.setThreaded(threaded);

	for (int i = 0; i < frame_range - 1; ++i) {
		try {
			if (time_step_t % 100 == 0) {
				/* No computations to be done, just copy the frame and update
				 * time scale */
				copyFile(files[i], r_files[frame++]);
				time_step_t += time_step;
			}

			retimer.setTimeScale((time_step_t % 100) * 0.01f);
			retimer.retime(files[i], files[i + 1], r_files[frame++]);

			/* Update time scale */
			time_step_t += time_step;
		}
		catch (...) {
			catch_exception();
		}

		std::cout << "Processing frame " << i << " ("<< i*100/frame_range << "%).\r" << std::flush;
	}

	std::cout << std::endl;
	std::cout << "Number of generated frames: " << frame << std::endl;
	std::cout << "Time: " << time_dt() - time_start << std::endl;
}

#endif
