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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include <GL/glew.h>

#include "application.hh"

#include <chrono>
#include <queue>

#include "bibliotheques/outils/definitions.hh"

#include "editrice_nulle.hh"

/* ************************************************************************** */

static constexpr auto TEMPS_DOUBLE_CLIC_MS = 500;

/* Maiximum, au cas où on laisse les utilisateurs renseigner le temps. */
//static constexpr auto TEMPS_DOUBLE_CLIC_MS_MAX = 5000;

static auto compte_tick_ms()
{
	auto const maintenant = std::chrono::system_clock::now();
	auto const duree = maintenant.time_since_epoch();
	return std::chrono::duration_cast<std::chrono::milliseconds>(duree).count();
}

/* ************************************************************************** */

static std::queue<Evenement> VG_queue_evenement{};

static void rappel_erreur(int error, const char* description)
{
	fprintf(stderr, "Error %d : %s\n", error, description);
}

static void rappel_clavier(
		GLFWwindow *fenetre,
		int cle,
		int scancode,
		int action,
		int mods)
{
	auto evenement = Evenement{};

	if (action == GLFW_PRESS) {
		evenement.type = evenement_fenetre::CLE_PRESSEE;
	}
	else if (action == GLFW_RELEASE) {
		evenement.type = evenement_fenetre::CLE_RELACHEE;
	}
	else if (action == GLFW_REPEAT) {
		evenement.type = evenement_fenetre::CLE_REPETEE;
	}

	evenement.cle = static_cast<type_cle>(cle);
	evenement.mods = static_cast<type_mod>(mods);

	VG_queue_evenement.push(evenement);

	INUTILISE(fenetre);
	INUTILISE(scancode);
}

static void rappel_position_souris(
		GLFWwindow *fenetre,
		double pos_x,
		double pos_y)
{
	auto evenement = Evenement{};
	evenement.type = evenement_fenetre::SOURIS_BOUGEE;
	evenement.pos.x = pos_x;
	evenement.pos.y = pos_y;

	VG_queue_evenement.push(evenement);

	INUTILISE(fenetre);
}

static void rappel_bouton_souris(
		GLFWwindow *fenetre,
		int bouton,
		int action,
		int mods)
{
	/* À FAIRE : double clic. */
	auto evenement = Evenement{};

	if (action == GLFW_PRESS) {
		evenement.type = evenement_fenetre::SOURIS_PRESSEE;
	}
	else if (action == GLFW_RELEASE) {
		evenement.type = evenement_fenetre::SOURIS_RELACHEE;
	}

	evenement.souris = static_cast<bouton_souris>(bouton);
	evenement.mods = static_cast<type_mod>(mods);

	VG_queue_evenement.push(evenement);

	INUTILISE(fenetre);
}

static void rappel_roulette(
		GLFWwindow *fenetre,
		double delta_x,
		double delta_y)
{
	auto evenement = Evenement{};
	evenement.type = evenement_fenetre::SOURIS_ROULETTE;
	evenement.delta.x = delta_x;
	evenement.delta.y = delta_y;

	VG_queue_evenement.push(evenement);

	INUTILISE(fenetre);
}

static void rappel_dimension(
		GLFWwindow *fenetre,
		int x,
		int y)
{
	glViewport(0, 0, x, y);

	auto evenement = Evenement{};
	evenement.type = evenement_fenetre::REDIMENSION;
	evenement.pos.x = x;
	evenement.pos.y = y;

	VG_queue_evenement.push(evenement);

	INUTILISE(fenetre);
}

/* ************************************************************************** */

void Application::joue()
{
	initialise_fenetre();
#ifdef AVEC_VULKAN
	initialise_vulkan();
#else
	initialise_opengl();
#endif
	boucle_principale();
	nettoye();
}

void Application::initialise_fenetre()
{
	if (glfwInit() == 0) {
		throw std::runtime_error("Impossible d'initialiser GLFW !");
	}

	glfwSetErrorCallback(rappel_erreur);

#ifdef AVEC_VULKAN
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#else
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

//	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	const int WIDTH = 800;
	const int HEIGHT = 600;

	std::cerr << "Avant glfwCreateWindow\n";
	fenetre = glfwCreateWindow(WIDTH, HEIGHT, "Créathèque", nullptr, nullptr);

	std::cerr << "Après glfwCreateWindow\n";
	if (fenetre == nullptr) {
		glfwTerminate();
		throw std::runtime_error("Impossible de créer une fenêtre GLFW !");
	}

	glfwSetKeyCallback(fenetre, rappel_clavier);
	glfwSetCursorPosCallback(fenetre, rappel_position_souris);
	glfwSetMouseButtonCallback(fenetre, rappel_bouton_souris);
	glfwSetScrollCallback(fenetre, rappel_roulette);
	glfwSetWindowSizeCallback(fenetre, rappel_dimension);

	m_editrices.push_back(new EditriceNulle);
}

#ifdef AVEC_VULKAN
void Application::initialise_vulkan()
{
	cree_instance();
}

void Application::cree_instance()
{
	auto appInfo = VkApplicationInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Application";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	auto createInfo = VkInstanceCreateInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	uint32_t glfwExtensionCount = 0;

	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	createInfo.enabledExtensionCount = glfwExtensionCount;
	createInfo.ppEnabledExtensionNames = glfwExtensions;

	createInfo.enabledLayerCount = 0;

	auto resultat = vkCreateInstance(&createInfo, nullptr, &instance);

	if (resultat != VK_SUCCESS) {
		throw std::runtime_error("failed to create instance!");
	}
}
#else
void Application::initialise_opengl()
{
	std::cerr << "Avant glfwMakeContextCurrent\n";
	glfwMakeContextCurrent(fenetre);
	std::cerr << "Après glfwMakeContextCurrent\n";
//	gladLoadGLLoader( glfwGetProcAddress);

	glewExperimental = GL_TRUE;
	auto const &erreur = glewInit();

	if (erreur != GLEW_OK) {
		std::cerr << "Erreur lors de l'initialisation du canevas OpenGL : "
				  << glewGetErrorString(erreur)
				  << '\n';
	}

	glfwSwapInterval(1);
}
#endif

void Application::boucle_principale()
{
	while (!glfwWindowShouldClose(fenetre)) {
		glfwWaitEvents();

		traite_evenements();

		dessine_fenetre();
	}
}

void Application::traite_evenements()
{
	while (!VG_queue_evenement.empty()) {
		auto evenement = VG_queue_evenement.front();
		VG_queue_evenement.pop();

		/* enregistrement de la position de la souris, car ceci n'est donnée
		 * que pour les mouvements */
		if (evenement.type == evenement_fenetre::SOURIS_BOUGEE) {
			souris = evenement.pos;
		}
		else if (evenement.type != evenement_fenetre::REDIMENSION) {
			evenement.pos = souris;
		}

		// si souris relachée
		// -- si dernier évènement est souris pressée -> nous avons un clic, ignore
		// -- sinon -> nous avons une souris relachée
		// si souris cliquée
		// -- si dernier clic < TEMPS_DOUBLE_CLIC -> nous avons un double clic

		/* Editrice::souris_clic() est toujours appelé quand la souris est cliquée
		 * Editrice::double_clic() est appelé si un deuxième clic survient dans l'écart de temps défini
		 * Editrice::souris_relachee() est appelé un autre évènement est survenu depuis le dernier clic
		 *    par exemple : presse souris, tappe une lettre, relache souris
		 */

		if (evenement.type == evenement_fenetre::SOURIS_RELACHEE) {
			if (dernier_evenement == evenement_fenetre::SOURIS_PRESSEE || dernier_evenement == evenement_fenetre::DOUBLE_CLIC) {
				continue;
			}

			/* traite l'évènement */
		}
		else if (evenement.type == evenement_fenetre::SOURIS_PRESSEE) {
			if (compte_tick_ms() - temps_double_clic <= TEMPS_DOUBLE_CLIC_MS) {
				evenement.type = evenement_fenetre::DOUBLE_CLIC;
			}

			temps_double_clic = compte_tick_ms();
		}

		traite_evenement(evenement);

		dernier_evenement = evenement.type;
	}
}

void Application::traite_evenement(Evenement const &evenement)
{
	std::cerr << "Évènement : " << evenement << '\n';

	for (auto &editrice : m_editrices) {
		if (editrice->accepte_evenement(evenement)) {
			break;
		}
	}
}

void Application::dessine_fenetre()
{
	/* Render here */
	glClear(GL_COLOR_BUFFER_BIT);

	glClearColor(0.5f, 0.5f, 1.0f, 1.0f);

	for (auto &editrice : m_editrices) {
		editrice->dessine();
	}

	/* Swap front and back buffers */
	glfwSwapBuffers(fenetre);

	/* Poll for and process events */
	glfwPollEvents();
}

void Application::nettoye()
{
	for (auto &editrice : m_editrices) {
		delete editrice;
	}

#ifdef AVEC_VULKAN
	vkDestroyInstance(instance, nullptr);
#endif
	glfwDestroyWindow(fenetre);
	glfwTerminate();
}
