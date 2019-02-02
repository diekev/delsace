/****************************************************************************
**
 **Copyright (C) 2014
**
 **This file is generated by the Magus toolkit
**
 **THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 **"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 **LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 **A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 **OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 **SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 **LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 **DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 **THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 **(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 **OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
****************************************************************************/

#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QGraphicsView>
#pragma GCC diagnostic pop

#include "base_editeur.h"

#include "coeur/scene.h"

class ObjectNodeItem;
class RepondantCommande;
class QGraphicsItem;
class QGraphicsRectItem;
class QGraphicsSceneMouseEvent;
class QMenu;
class QtConnexion;
class QtNode;
class QtNodeGraphicsScene;
class VueEditeurNoeud;

namespace danjo {

class GestionnaireInterface;

}  /* namespace danjo */

class EditriceGraphe : public BaseEditrice {
	Q_OBJECT

	VueEditeurNoeud *m_view;
	QtNodeGraphicsScene *m_graphics_scene;

	QGraphicsRectItem *m_rubber_band;
	QPointF m_last_mouse_position;

	bool m_rubberband_selection;
	bool m_mouse_down = false;

	/* Cached informations */
	QtConnexion *m_hover_connexion;
	QtConnexion *m_active_connexion;
	QVector<QtNode *> m_selected_nodes;
	QVector<QtConnexion *> m_selected_connexions;

public:
	explicit EditriceGraphe(
			RepondantCommande *repondant,
			danjo::GestionnaireInterface *gestionnaire,
			QWidget *parent = nullptr);

	EditriceGraphe(EditriceGraphe const &) = default;
	EditriceGraphe &operator=(EditriceGraphe const &) = default;

	virtual ~EditriceGraphe() override;

	/* Interface */

	/**
	 * Redessine les noeuds selon l'état du modèle.
	 */
	void update_state(type_evenement event) override;

	/**
	 * Installe le pointeur vers le menu de création de noeuds dans la vue
	 * de l'éditeur.
	 */
	void menu_ajout_noeud(QMenu *menu);

	/* À FAIRE */

	/* If there is a selected node, return the last one that was selected. */
	QtNode *getLastSelectedNode() const;

	/* Moves a node to front; before all other nodes. */
	void toFront(QtNode *node);

	/* Moves all other nodes in front of the given node. */
	void toBack(QtNode *node);

	QtConnexion *nodeOverConnexion(QtNode *node);

	void sendNotification() const;

public Q_SLOTS:
	/* Activated when a connexion is set between two nodes. */
	void connexionEstablished(QtConnexion*);

private:
	/* Called when a node is dropped on a connexion. */
	void splitConnexionWithNode(QtNode *node);

	/* Called when an object node is selected. */
	void setActiveObject(ObjectNodeItem *node);

	/* Called when nodes are connected. */
	void nodesConnected(QtNode *from, QString const &socket_from, QtNode *to, QString const &socket_to, bool notify);

	/* Called when nodes are disconnected. */
	void connexionRemoved(QtNode *from, QString const &socket_from, QtNode *to, QString const &socket_to, bool notify);

	void enterObjectNode(QAction *action);

protected:
	/* Event handling */
	bool eventFilter(QObject *object, QEvent *event) override;

	bool mouseClickHandler(QGraphicsSceneMouseEvent *mouseEvent);
	bool mouseDoubleClickHandler(QGraphicsSceneMouseEvent *mouseEvent);
	bool mouseMoveHandler(QGraphicsSceneMouseEvent *mouseEvent);
	bool mouseReleaseHandler(QGraphicsSceneMouseEvent *mouseEvent);

	bool ctrlPressed();

	QGraphicsItem *itemAtExceptActiveConnexion(QPointF const &pos);

	void rubberbandSelection(QGraphicsSceneMouseEvent *mouseEvent);
	void deselectAll();
	void deleteAllActiveConnexions();
	void deselectConnexions();
	void deselectNodes();
	void selectNode(QtNode *node, QGraphicsSceneMouseEvent *mouseEvent);
	void selectConnexion(QtConnexion *connexion);

	QtNode *nodeWithActiveConnexion();

	bool isAlreadySelected(QtNode *node);
	bool isAlreadySelected(QtConnexion *connexion);
};
