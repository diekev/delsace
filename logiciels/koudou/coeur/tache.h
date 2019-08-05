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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <memory>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QObject>
#pragma GCC diagnostic pop

#include <tbb/task.h>

namespace kdo {
class Koudou;
}
class FenetrePrincipale;

/* Apparemment nous ne pouvons pas dériver un objet depuis QObject et tbb::task
 * simultanément, donc cette classe fait le pont entre une tâche tbb et
 * l'entreface utilisateur pour notifier celle-ci de certains évènements.
 */
class NotaireTache : public QObject {
	Q_OBJECT

public:
	explicit NotaireTache(FenetrePrincipale *fenetre);

	void signale_rendu_fini();
	void signale_tache_commence();
	void signale_tache_fini();
	void signale_progres_avance(float progres);
	void signale_progres_temps(unsigned int echantillon, double temps_echantillon, double temps_ecoule, double temps_restant);

Q_SIGNALS:
	void rendu_fini();

	void tache_commence();
	void tache_fini();

	void progres_avance(float progres);
	void progres_temps(unsigned int echantillon, double temps_echantillon, double temps_ecoule, double temps_restant);
};

class Tache : public tbb::task {
protected:
	std::unique_ptr<NotaireTache> m_notaire = nullptr;
	kdo::Koudou const &m_koudou;

public:
	explicit Tache(kdo::Koudou const &koudou);

	tbb::task *execute() override;

	virtual void commence(kdo::Koudou const &koudou) = 0;
};
