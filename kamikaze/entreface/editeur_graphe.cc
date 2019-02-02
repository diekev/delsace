/****************************************************************************
**
* *Copyright (C) 2014
**
* *This file is generated by the Magus toolkit
**
* *THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* *"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* *LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
* *A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
* *OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* *SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* *LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* *DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
* *THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* *(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* *OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
****************************************************************************/

#include "editeur_graphe.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QApplication>
#include <QToolTip>
#include <QVBoxLayout>

#include <iostream>
#include "sdk/operatrice.h"
#include <sstream>

#include "nodes/node_compound.h"
#include "nodes/node_connection.h"
#include "nodes/node_node.h"
#include "nodes/node_port.h"
#include "nodes/node_scene.h"
#include "nodes/vue_editeur_noeud.h"

#include "object.h"
#include "graphs/object_graph.h"

/* ************************************************************************** */

EditriceGraphe::EditriceGraphe(
		RepondantCommande *repondant,
		danjo::GestionnaireInterface *gestionnaire,
		QWidget *parent)
	: BaseEditrice(parent)
	, m_view(new VueEditeurNoeud(repondant, gestionnaire, this))
	, m_graphics_scene(new QtNodeGraphicsScene())
{
	m_main_layout->addWidget(m_view);

	m_graphics_scene->installEventFilter(this);

	/* Hide graphics view's frame. */
	m_view->setStyleSheet("border: 0px");
	m_view->setScene(m_graphics_scene);
	m_view->setRenderHint(QPainter::Antialiasing, true);
	m_view->setInteractive(true);
	m_view->setBackgroundBrush(QBrush(QColor(127, 127, 127)));

	m_rubber_band = nullptr;
	m_hover_connexion = nullptr;
	m_active_connexion = nullptr;
	m_rubberband_selection = false;

	setContextMenuPolicy(Qt::CustomContextMenu);
}

EditriceGraphe::~EditriceGraphe()
{
	delete m_graphics_scene;
}

QGraphicsItem *EditriceGraphe::itemAtExceptActiveConnexion(QPointF const &pos)
{
	auto const &items = m_graphics_scene->items(QRectF(pos - QPointF(1, 1), QSize(3, 3)));
	auto const is_active = (m_active_connexion != nullptr);

	/* If there is an active connexion, it is not returned as a selected item.
	 * Finalized (established) connexions are returned. */
	for (auto const &item : items) {
		if (!item->isVisible()) {
			continue;
		}

		if (!is_connexion(item)) {
			return item;
		}

		if (!is_active) {
			return item;
		}
	}

	return nullptr;
}

QtConnexion *EditriceGraphe::nodeOverConnexion(QtNode *node)
{
	if (!node->hasInputs() || !node->hasOutputs()) {
		return nullptr;
	}

	/* already connected */
	if (node->input(0)->isConnected() || node->output(0)->isConnected()) {
		return nullptr;
	}

	auto check_hover = [&](QtNode *nitem, QtConnexion *citem)
	{
		auto const &bport_pos_x = citem->getBasePort()->scenePos().x();
		auto const &bport_pos_y = citem->getBasePort()->scenePos().y();
		auto const &tport_pos_x = citem->getTargetPort()->scenePos().x();
		auto const &tport_pos_y = citem->getTargetPort()->scenePos().y();

		auto const &min_x = std::min(bport_pos_x, tport_pos_x);
		auto const &min_y = std::min(bport_pos_y, tport_pos_y);
		auto const &max_x = std::max(bport_pos_x, tport_pos_x);
		auto const &max_y = std::max(bport_pos_y, tport_pos_y);

		auto const &pos_x = nitem->pos().x();
		auto const &pos_y = nitem->pos().y();

		return (min_x < pos_x && max_x > pos_x) && (min_y < pos_y && max_y > pos_y);
	};

	if (m_hover_connexion && check_hover(node, m_hover_connexion)) {
		return m_hover_connexion;
	}

	QtConnexion *connexion;

	for (auto const &item : m_graphics_scene->items()) {
		if (!is_connexion(item)) {
			continue;
		}

		connexion = static_cast<QtConnexion *>(item);

		if (connexion->isSelected()) {
			continue;
		}

		if (node->isConnexionConnectedToThisNode(connexion)) {
			continue;
		}

		if (check_hover(node, connexion)) {
			return connexion;
		}
	}

	if (m_hover_connexion) {
		m_hover_connexion->setSelected(false);
	}

	return nullptr;
}

bool EditriceGraphe::eventFilter(QObject *object, QEvent *event)
{
	auto mouseEvent = static_cast<QGraphicsSceneMouseEvent *>(event);

	switch (static_cast<int>(event->type())) {
		case QEvent::GraphicsSceneMousePress:
		{
			this->set_active();
			return mouseClickHandler(mouseEvent);
		}
		case QEvent::GraphicsSceneMouseDoubleClick:
		{
			this->set_active();

			if (mouseDoubleClickHandler(mouseEvent)) {
				return true;
			}

			break;
		}
		case QEvent::GraphicsSceneMouseMove:
		case QEvent::GraphicsSceneDragEnter:
		case QEvent::GraphicsSceneDragMove:
		{
			return mouseMoveHandler(mouseEvent);
		}
		case QEvent::GraphicsSceneDragLeave:
		case QEvent::GraphicsSceneMouseRelease:
		{
			return mouseReleaseHandler(mouseEvent);
		}
	}

	return QObject::eventFilter(object, event);
}

bool EditriceGraphe::mouseClickHandler(QGraphicsSceneMouseEvent *mouseEvent)
{
	switch (static_cast<int>(mouseEvent->button())) {
		case Qt::LeftButton:
		{
			m_mouse_down = true;

			auto const &item = itemAtExceptActiveConnexion(mouseEvent->scenePos());

			if (!item) {
				/* Left-click on the canvas, but no item clicked, so deselect nodes and connexions */
				deselectAll();
				m_rubberband_selection = true;
				m_last_mouse_position.setX(mouseEvent->lastScenePos().x());
				m_last_mouse_position.setY(mouseEvent->lastScenePos().y());
				return true;
			}

			m_rubberband_selection = false;

			/* Did not click on a node. */
			if (!item->data(NODE_KEY_GRAPHIC_ITEM_TYPE).isValid()) {
				/* TODO: shouldn't we return false? */
				return true;
			}

			/* Delegate to the node; either the node itself is clicked, one of
			 * its children or a connexion.
			 */

			auto const type = item->data(NODE_KEY_GRAPHIC_ITEM_TYPE).toInt();

			switch (type) {
				case NODE_VALUE_TYPE_CONNECTION:
				{
					deselectNodes();
					selectConnexion(static_cast<QtConnexion *>(item));
					break;
				}
				case NODE_VALUE_TYPE_PORT:
				{
					deselectNodes();
					deselectConnexions();

					auto node = static_cast<QtNode *>(item->parentItem());

					/* Either make a connexion to another port, or create a new
					 * connexion */
					auto baseNode = nodeWithActiveConnexion();

					if (m_active_connexion == nullptr) {
						/* There is no active connexion, so start one */
						node->mouseLeftClickHandler(mouseEvent, item, NODE_ACTION_BASE);
						m_active_connexion = node->m_active_connexion;
					}
					else if (baseNode != node) {
						/* There is an active connexion and the selected
						 * port is not part of the baseNode, so try to
						 * establish a connexion with the other node */
						if (node->mouseLeftClickHandler(mouseEvent, item,
						                                NODE_ACTION_TARGET,
						                                baseNode->m_active_connexion))
						{
							m_active_connexion = nullptr;
						}
					}

					break;
				}
				default:
				{
					deselectNodes();
					deselectConnexions();

					auto node = static_cast<QtNode *>(item->parentItem());
					node->mouseLeftClickHandler(mouseEvent, item);

					break;
				}
			}

			break;
		}
		case Qt::MiddleButton:
		{
			auto const &item = itemAtExceptActiveConnexion(mouseEvent->scenePos());

			if (!item) {
				return true;
			}

			if (!item->data(NODE_KEY_GRAPHIC_ITEM_TYPE).isValid()) {
				/* TODO: shouldn't we return false? */
				return true;
			}

			auto const type = item->data(NODE_KEY_GRAPHIC_ITEM_TYPE).toInt();

			Noeud *noeud = nullptr;

			switch (type) {
				case NODE_VALUE_TYPE_NODE_BODY:
				case NODE_VALUE_TYPE_HEADER_TITLE:
				case NODE_VALUE_TYPE_HEADER_ICON:
				{
					noeud = static_cast<QtNode *>(item->parentItem())->pointeur_noeud();
					break;
				}
				case NODE_VALUE_TYPE_NODE:
				{
					noeud = static_cast<QtNode *>(item)->pointeur_noeud();
					break;
				}
				case NODE_VALUE_TYPE_CONNECTION:
				case NODE_VALUE_TYPE_PORT:
				default:
				{
					break;
				}
			}

			if (noeud != nullptr) {
				auto operatrice = noeud->operatrice();

				std::stringstream ss;
				ss << "<p>Opérateur : " << noeud->nom() << "</p>";
				ss << "<hr/>";
				ss << "<p>Temps d'exécution :";
				ss << "<p>- dernière : " << operatrice->temps_execution() << " secondes.</p>";
				ss << "<p>- minimum : " << operatrice->min_temps_execution() << " secondes.</p>";
				ss << "<p>- agrégé : " << operatrice->temps_agrege() << " secondes.</p>";
				ss << "<p>- minimum agrégé : " << operatrice->min_temps_agrege() << " secondes.</p>";
				ss << "<hr/>";
				ss << "<p>Nombre d'exécution : " << operatrice->nombre_executions() << "</p>";
				ss << "<hr/>";

				QToolTip::showText(mouseEvent->screenPos(), ss.str().c_str());
			}

			return true;
		}
	}

	return true;
}

bool EditriceGraphe::mouseDoubleClickHandler(QGraphicsSceneMouseEvent *mouseEvent)
{

	std::cerr << __func__ << '\n';

	switch (static_cast<int>(mouseEvent->button())) {
		case Qt::LeftButton:
		{
			auto const &item = itemAtExceptActiveConnexion(mouseEvent->scenePos());

			if (!item) {
				/* Double-click on the canvas. */
				return true;
			}

			/* Did not click on a node. */
			if (!item->data(NODE_KEY_GRAPHIC_ITEM_TYPE).isValid()) {
				/* TODO: shouldn't we return false? */
				return true;
			}

			/* Delegate to the node; either the node itself is clicked, one of
			 * its children or a connexion.
			 */

			auto const type = item->data(NODE_KEY_GRAPHIC_ITEM_TYPE).toInt();

			switch (type) {
				case NODE_VALUE_TYPE_NODE:
				{
					if (is_object_node(item)) {
						enterObjectNode(nullptr);
					}

					break;
				}
				case NODE_VALUE_TYPE_NODE_BODY:
				{
					if (is_object_node(item->parentItem())) {
						enterObjectNode(nullptr);
					}

					break;
				}
				case NODE_VALUE_TYPE_HEADER_TITLE:
				{
					/* This is handled in TextItem::mouseDoubleClickEvent. */
					return false;
				}
				default:
				{
					return false;
				}
			}
		}
	}

	return true;
}

void EditriceGraphe::enterObjectNode(QAction */*action*/)
{
	/* À FAIRE */
	m_context->eval_ctx->edit_mode = true;
	m_context->scene->notify_listeners(type_evenement::noeud | type_evenement::selectione);
}

bool EditriceGraphe::mouseMoveHandler(QGraphicsSceneMouseEvent *mouseEvent)
{
	if (!m_mouse_down) {
		return true;
	}

	/* If there was a rubberband selection started, update its rectangle */
	if (m_rubberband_selection && (mouseEvent->buttons() & Qt::LeftButton)) {
		rubberbandSelection(mouseEvent);
	}

	if (m_selected_nodes.size() == 1) {
		auto node = static_cast<QtNode *>(getLastSelectedNode());
		m_hover_connexion = nodeOverConnexion(node);

		if (m_hover_connexion) {
			/* TODO: this is to highlight the conection, consider having a
			 * separate flag/colour for this. */
			m_hover_connexion->setSelected(true);
			return true;
		}
	}

	if (m_active_connexion) {
		m_active_connexion->updatePath(mouseEvent->scenePos());
	}

	m_hover_connexion = nullptr;
	setCursor(Qt::ArrowCursor);

	return true;
}

bool EditriceGraphe::mouseReleaseHandler(QGraphicsSceneMouseEvent *mouseEvent)
{
	m_mouse_down = false;

	/* Determine whether a node has been dropped on a connexion. */
	if (m_hover_connexion) {
		splitConnexionWithNode(getLastSelectedNode());
		m_hover_connexion = nullptr;
		return true;
	}

	/* Handle the rubberband selection, if applicable */
	if (!m_rubberband_selection) {
		return false;
	}

	if (mouseEvent->button() & Qt::LeftButton) {
		if (m_rubber_band) {
			auto minX = qMin(m_last_mouse_position.x(), mouseEvent->lastScenePos().x());
			auto maxX = qMax(m_last_mouse_position.x(), mouseEvent->lastScenePos().x());
			auto minY = qMin(m_last_mouse_position.y(), mouseEvent->lastScenePos().y());
			auto maxY = qMax(m_last_mouse_position.y(), mouseEvent->lastScenePos().y());

			/* Select the items */
			auto const &items = m_graphics_scene->items();
			for (auto const &item : items) {
				if (!item->isVisible()) {
					continue;
				}

				if (is_connexion(item)) {
					auto connexion = static_cast<QtConnexion *>(item);
					auto prise_base = connexion->getBasePort();
					auto prise_cible = connexion->getTargetPort();

					if (prise_base && prise_cible) {
						auto const &pos_prise_base = prise_base->scenePos();
						auto const &pos_prise_cible = prise_cible->scenePos();

						auto item_Min_X = qMin(pos_prise_base.x(), pos_prise_cible.x());
						auto item_Max_X = qMax(pos_prise_base.x(), pos_prise_cible.x());
						auto item_Min_Y = qMin(pos_prise_base.y(), pos_prise_cible.y());
						auto item_Max_Y = qMax(pos_prise_base.y(), pos_prise_cible.y());

						if (item_Min_X > minX && item_Max_X < maxX &&
							item_Min_Y > minY && item_Max_Y < maxY)
						{
							selectConnexion(connexion);
						}
					}
				}
				else if (is_node(item)) {
					auto noeud = static_cast<QtNode *>(item);
					auto const &pos_neoud = noeud->scenePos();
					auto const &cadre_noeud = noeud->sceneBoundingRect();

					auto item_Min_X = pos_neoud.x() - 0.5 * cadre_noeud.width() + 10;
					auto item_Min_Y = pos_neoud.y() - 0.5 * cadre_noeud.height() + 10;
					auto item_Max_X = item_Min_X + cadre_noeud.width() + 10;
					auto item_Max_Y = item_Min_Y + cadre_noeud.height()  + 10;

					if (item_Min_X > minX && item_Max_X < maxX &&
						item_Min_Y > minY && item_Max_Y < maxY)
					{
						selectNode(noeud, nullptr);
					}
				}
			}

			m_rubber_band->hide();
		}

		m_rubberband_selection = false;
	}

	return true;
}

void EditriceGraphe::rubberbandSelection(QGraphicsSceneMouseEvent *mouseEvent)
{
	/* Mouse is pressed and moves => draw rubberband */
	auto const x = mouseEvent->lastScenePos().x();
	auto const y = mouseEvent->lastScenePos().y();

	if (!m_rubber_band) {
		m_rubber_band = new QGraphicsRectItem(m_last_mouse_position.x(), m_last_mouse_position.y(), 0.0f, 0.0f);
		m_rubber_band->setPen(QPen(Qt::darkBlue));
		QColor c(Qt::darkBlue);
		c.setAlpha(64);
		m_rubber_band->setBrush(c);
		m_graphics_scene->addItem(m_rubber_band);
	}

	m_rubber_band->show();

	auto const minX = std::min(static_cast<qreal>(m_last_mouse_position.x()), x);
	auto const maxX = std::max(static_cast<qreal>(m_last_mouse_position.x()), x);
	auto const minY = std::min(static_cast<qreal>(m_last_mouse_position.y()), y);
	auto const maxY = std::max(static_cast<qreal>(m_last_mouse_position.y()), y);

	m_rubber_band->setRect(minX, minY, maxX - minX, maxY - minY);
}

void EditriceGraphe::selectNode(QtNode *node, QGraphicsSceneMouseEvent *mouseEvent)
{
	if (!ctrlPressed()) {
		deselectAll();
	}

	if (is_object_node(node)) {
		setActiveObject(static_cast<ObjectNodeItem *>(node));
	}
	else {
		auto object = static_cast<Object *>(m_context->scene->active_node());

		auto graph = object->graph();
		graph->ajoute_selection(node->pointeur_noeud());

		m_context->scene->notify_listeners(type_evenement::noeud | type_evenement::selectione);
	}
}

void EditriceGraphe::selectConnexion(QtConnexion *connexion)
{
	if (!ctrlPressed()) {
		deselectAll();
	}

	if (!isAlreadySelected(connexion)) {
		auto object = static_cast<Object *>(m_context->scene->active_node());
		auto graph = object->graph();
		graph->ajoute_selection(connexion->pointeur_lien());

		m_selected_connexions.append(connexion);
		connexion->setSelected(true);
	}
}

void EditriceGraphe::deselectAll()
{
	setCursor(Qt::ArrowCursor);
	deleteAllActiveConnexions();
	deselectConnexions();
	deselectNodes();
}

void EditriceGraphe::deleteAllActiveConnexions()
{
	auto const &items = m_graphics_scene->items();
	QtNode *node;

	for (auto const &item : items) {
		if (is_node(item) && item->isVisible()) {
			node = static_cast<QtNode *>(item);
			node->deleteActiveConnexion();
		}
	}

	m_active_connexion = nullptr;
}

void EditriceGraphe::deselectConnexions()
{
	for (auto const &connexion : m_selected_connexions) {
		if (connexion->isVisible()) {
			connexion->setSelected(false);
		}
	}

	m_selected_connexions.clear();
}

void EditriceGraphe::deselectNodes()
{
	if (m_context->eval_ctx->edit_mode) {
		auto object = static_cast<Object *>(m_context->scene->active_node());
		auto graph = object->graph();

		for (auto const &node : m_selected_nodes) {
			graph->enleve_selection(node->pointeur_noeud());
		}
	}
	else {
		/* TODO */
	}

	m_selected_nodes.clear();
}

QtNode *EditriceGraphe::nodeWithActiveConnexion()
{
	if (!m_active_connexion) {
		return nullptr;
	}

	return static_cast<QtNode *>(m_active_connexion->getBasePort()->parentItem());
}

using node_port_pair = std::pair<QtNode *, QtPort *>;

std::pair<node_port_pair, node_port_pair> get_base_target_pairs(QtConnexion *connexion, bool remove)
{
	auto base_port = connexion->getBasePort();
	auto target_port = connexion->getTargetPort();

	if (remove) {
		base_port->deleteConnexion(connexion);
	}

	/* make sure the connexion is set in the right order */
	if (target_port->isOutputPort() && !base_port->isOutputPort()) {
		std::swap(base_port, target_port);
	}

	auto const &base_node = static_cast<QtNode *>(base_port->parentItem());
	auto const &target_node = static_cast<QtNode *>(target_port->parentItem());

	return {
		{ base_node, base_port },
		{ target_node, target_port }
	};
}

void EditriceGraphe::splitConnexionWithNode(QtNode *node)
{
	auto const &connexion = m_hover_connexion;

	auto const &pairs = get_base_target_pairs(connexion, true);
	auto const &base = pairs.first;
	auto const &target = pairs.second;

	auto scene = m_context->scene;
	auto object = static_cast<Object *>(scene->active_node());
	auto graph = object->graph();

	auto noeud_de = base.first->pointeur_noeud();
	auto noeud_a = target.first->pointeur_noeud();

	/* remove connexion */
	auto prise_sortie = noeud_de->sortie(base.second->getPortName().toStdString());
	auto prise_entree = noeud_a->entree(target.second->getPortName().toStdString());

	assert((prise_sortie != nullptr) && (prise_entree != nullptr));

	graph->deconnecte(prise_sortie, prise_entree);

	auto noeud_milieu = node->pointeur_noeud();

	/* connect from base port to first input port in node */
	graph->connecte(prise_sortie, noeud_milieu->entree(0));

	/* connect from first output port in node to target port */
	graph->connecte(noeud_milieu->sortie(0), prise_entree);

	/* notify */
	scene->evalObjectDag(*m_context, object);
	scene->notify_listeners(type_evenement::noeud | type_evenement::modifie);
}

void EditriceGraphe::connexionEstablished(QtConnexion *connexion)
{
	auto const &pairs = get_base_target_pairs(connexion, false);
	auto const &base = pairs.first;
	auto const &target = pairs.second;

	nodesConnected(base.first, base.second->getPortName(),
	               target.first, target.second->getPortName(), true);
}

QtNode *EditriceGraphe::getLastSelectedNode() const
{
	if (m_selected_nodes.empty()) {
		return nullptr;
	}

	return m_selected_nodes.back();
}

void EditriceGraphe::menu_ajout_noeud(QMenu *menu)
{
	m_view->menu_ajout_noeud(menu);
}

void EditriceGraphe::toFront(QtNode *node)
{
	if (!node) {
		return;
	}

	auto const &items = m_graphics_scene->items();

	/* First set the node in front of all other nodes */
	for (auto const &item : items) {
		if (node != item && is_node(item) && item->isVisible()) {
			item->stackBefore(node);
		}
	}

	/* Put the connexions of the node in front of the node and the other connexions behind the node */
	for (auto const &item : items) {
		if (!is_node(item)) {
			continue;
		}

		auto const &connexion = static_cast<QtConnexion *>(item);

		if (node->isConnexionConnectedToThisNode(connexion)) {
			node->stackBefore(item);
		}
		else {
			item->stackBefore(node);
		}
	}
}

void EditriceGraphe::toBack(QtNode *node)
{
	if (!node) {
		return;
	}

	auto const &items = m_graphics_scene->items();

	/* Set all other nodes in front of this node */
	for (auto const &item : items) {
		if (node != item && is_node(item) && item->isVisible()) {
			node->stackBefore(item);
		}
	}
}

bool EditriceGraphe::ctrlPressed()
{
	return (QGuiApplication::keyboardModifiers() & Qt::ControlModifier);
}

bool EditriceGraphe::isAlreadySelected(QtNode *node)
{
	auto iter = std::find(m_selected_nodes.begin(),
	                      m_selected_nodes.end(),
	                      node);

	return (iter != m_selected_nodes.end());
}

bool EditriceGraphe::isAlreadySelected(QtConnexion *connexion)
{
	auto iter = std::find(m_selected_connexions.begin(),
	                      m_selected_connexions.end(),
	                      connexion);

	return (iter != m_selected_connexions.end());
}

void EditriceGraphe::update_state(type_evenement event)
{
	if (event == static_cast<type_evenement>(-1)) {
		return;
	}

	/* Clear all the node in the scene. */
	m_graphics_scene->clear();
	m_graphics_scene->items().clear();
	assert(m_graphics_scene->items().size() == 0);
	m_selected_nodes.clear();
	m_selected_connexions.clear();

	/* Add nodes to the scene. */

	/* Add the object's graph's nodes to the scene. */
	if (m_context->eval_ctx->edit_mode) {
		auto scene_node = m_context->scene->active_node();

		if (scene_node == nullptr) {
			return;
		}

		std::unordered_map<Noeud *, QtNode *> node_items_map;

		auto object = static_cast<Object *>(scene_node);
		auto graph = object->graph();

		/* Add the nodes. */
		for (auto const &noeud : graph->noeuds()) {
			Noeud *pointeur_noeud = noeud.get();

			auto node_item = new QtNode(pointeur_noeud->nom().c_str());
			node_item->setTitleColor(Qt::white);
			node_item->alignTitle(ALIGNED_LEFT);
			node_item->pointeur_noeud(pointeur_noeud);
			node_item->setScene(m_graphics_scene);
			node_item->setEditor(this);
			node_item->setPos(pointeur_noeud->posx(), pointeur_noeud->posy());

			node_items_map[pointeur_noeud] = node_item;

			m_graphics_scene->addItem(node_item);

			if (pointeur_noeud->a_drapeau(NOEUD_SELECTIONE)) {
				m_selected_nodes.append(node_item);
			}
		}

		/* Add the connexions. */
		for (auto const &lien : graph->liens()) {
			PriseSortie *sortie = lien->sortie;
			PriseEntree *entree = lien->entree;

			QtNode *from_node_item = node_items_map[sortie->parent];
			QtNode *to_node_item = node_items_map[entree->parent];

			QtPort *from_port = from_node_item->output(sortie->nom.c_str());
			QtPort *to_port = to_node_item->input(entree->nom.c_str());

			assert(from_port != nullptr && to_port != nullptr);

			from_node_item->createActiveConnexion(from_port, from_port->pos());
			auto item = to_port->createConnexion(from_node_item->m_active_connexion);

			item->pointeur_lien(lien);

			if (lien->a_drapeau(NOEUD_SELECTIONE)) {
				item->setSelected(true);
			}

			from_node_item->m_active_connexion = nullptr;
		}

		/* Add the children of this object. */
	}
	/* Add the object nodes to the scene. */
	else {
		for (auto const &node : m_context->scene->nodes()) {
			auto object = static_cast<Object *>(node.get());

			if (!object) {
				continue;
			}

			/* If it is has a parent, skip. */
			if (object->parent()) {
				continue;
			}

			auto node_item = new ObjectNodeItem(node.get(), node->name().c_str());
			node_item->setTitleColor(Qt::white);
			node_item->alignTitle(ALIGNED_CENTER);

			node_item->setEditor(this);
			node_item->setScene(m_graphics_scene);
			node_item->setPos(node->xpos(), node->ypos());

#if 0
			auto rect = node_item->boundingRect();

			std::cerr << "Position : " << node->xpos() << ", " << node->ypos() << "\n";
			std::cerr << "Rectangle (Qt) : " << rect.x() << ", " << rect.y() << ", "
					  << rect.width() << ", " << rect.height() << "\n";
#endif

			if (node.get() == m_context->scene->active_node()) {
				m_selected_nodes.append(node_item);
			}

			m_graphics_scene->addItem(node_item);
		}
	}

	/* Make sure selected items are highlighted and in front of others. */
	for (auto node_item : m_selected_nodes) {
		node_item->setSelected(true);
		toFront(node_item);
	}
}

void EditriceGraphe::setActiveObject(ObjectNodeItem *node)
{
	m_context->scene->set_active_node(node->scene_node());
}

void EditriceGraphe::nodesConnected(QtNode *from, QString const &socket_from, QtNode *to, QString const &socket_to, bool notify)
{
	auto scene = m_context->scene;

	if (m_context->eval_ctx->edit_mode) {
		auto object = static_cast<Object *>(scene->active_node());
		auto graph = object->graph();

		auto noeud_de = from->pointeur_noeud();
		auto noeud_a = to->pointeur_noeud();

		auto prise_sortie = noeud_de->sortie(socket_from.toStdString());
		auto prise_entree = noeud_a->entree(socket_to.toStdString());

		assert((prise_sortie != nullptr) && (prise_entree != nullptr));

		graph->connecte(prise_sortie, prise_entree);

		/* Needed to prevent updating needlessly the graph when dropping a node on
		 * a connexion. */
		if (notify) {
			scene->evalObjectDag(*m_context, object);
			scene->notify_listeners(type_evenement::noeud | type_evenement::modifie);
		}
	}
	else {
		auto node_from = static_cast<ObjectNodeItem *>(from)->scene_node();
		auto node_to = static_cast<ObjectNodeItem *>(to)->scene_node();

		scene->connect(*m_context, node_from, node_to);
		scene->notify_listeners(type_evenement::objet | type_evenement::parente);
	}
}

void EditriceGraphe::connexionRemoved(QtNode *from, QString const &socket_from, QtNode *to, QString const &socket_to, bool notify)
{
	auto scene = m_context->scene;

	if (m_context->eval_ctx->edit_mode) {
		auto object = static_cast<Object *>(scene->active_node());
		auto graph = object->graph();

		auto noeud_de = from->pointeur_noeud();
		auto noeud_a = to->pointeur_noeud();

		auto prise_sortie = noeud_de->sortie(socket_from.toStdString());
		auto prise_entree = noeud_a->entree(socket_to.toStdString());

		assert((prise_sortie != nullptr) && (prise_entree != nullptr));

		graph->deconnecte(prise_sortie, prise_entree);

		if (notify) {
			scene->evalObjectDag(*m_context, object);
			scene->notify_listeners(type_evenement::noeud | type_evenement::modifie);
		}
	}
	else {
		auto node_from = static_cast<ObjectNodeItem *>(from)->scene_node();
		auto node_to = static_cast<ObjectNodeItem *>(to)->scene_node();

		scene->disconnect(*m_context, node_from, node_to);
		scene->notify_listeners(type_evenement::objet | type_evenement::parente);
	}
}

void EditriceGraphe::sendNotification() const
{
	auto scene = m_context->scene;
	scene->notify_listeners(type_evenement::noeud | type_evenement::modifie);
}
