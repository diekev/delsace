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
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "profilage.hh"

void imprime_profilage(std::ostream &os)
{
	Prof_update(1);

	Prof_set_report_mode(Prof_Report_Mode::Prof_SELF_TIME);

	auto pob = Prof_create_report();

	for (auto i = 0; i < pob->num_record; ++i) {
		auto record = pob->record[i];

		for (auto j = 0; j < record.indent; ++j) {
			os << ' ';
		}

		if (record.prefix) {
			os << record.prefix << ' ';
		}

		os << record.name << ' ';

		for (auto j = 0; j < 4; ++j) {
			if (record.value_flag & (1 << j)) {
				os << record.values[j] * 100.0 << ' ';
			}
		}

		os << '\n';
	}

	Prof_free_report(pob);
}
