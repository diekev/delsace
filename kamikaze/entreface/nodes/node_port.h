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

#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QFont>
#pragma GCC diagnostic pop

#include "node_connection.h"
#include "node_constants.h"

static constexpr auto NODE_PORT_TYPE_INPUT = 1; /* Reserved for input port */
static constexpr auto NODE_PORT_TYPE_OUTPUT = 2; /* Reserved for output port */

static constexpr auto NODE_PORT_FONT_SIZE = 10;
static constexpr auto NODE_PORT_SHAPE_SIZE = 10;
static constexpr auto NODE_PORT_OFFSET = 10.0;
static constexpr auto NODE_PORT_WIDTH_MARGIN = 10.0; /* Margin in pixel value */
static constexpr auto NODE_PORT_HEIGHT_MARGIN_FACTOR = 0.8; /* Margin factor in fraction of text height */

/****************************************************************************
 * QtPort represents a Port-class which is visualised in a QtNode.
 * A QtPort can be connected to another port under certain conditions. These
 * conditions are defined by mean of a Policy
 ***************************************************************************/
class QtPort : public QGraphicsPathItem {
	QString m_port_name;
	int m_port_type;
	QColor m_port_colour;
	QColor m_connexion_colour;
	Alignment m_alignment;
	QGraphicsTextItem *m_label;
	QFont m_font;
	QPointF m_original_pos;

	QVector<QtConnexion *> m_connexions;

public:
	QtPort(QString const &portName,
	       int portType,
	       QColor const &portColour,
	       QColor const &connexionColour,
	       Alignment alignment,
	       QGraphicsItem *parent = nullptr);

	QtPort(QtPort const &) = default;
	QtPort &operator=(QtPort const &) = default;

	~QtPort() = default;

	/* Redraw the port */
	void redraw();

	/* Set the color of the portName */
	void setNameColor(QColor const &color);

	/* Returns the width when the port is not zoomed */
	qreal getNormalizedWidth();

	/* Returns the height when the port is not zoomed */
	qreal getNormalizedHeight();

	/* Set the position according to its */
	void setAlignedPos(QPointF const &pos);
	void setAlignedPos(qreal x, qreal y);

	/* Create a connexion on this port. The port acts as base or a target */
	QtConnexion *createConnexion(QtConnexion *targetConnexion = nullptr);

	/* Delete the connexion of this port. */
	void deleteConnexion(QtConnexion *connexion, bool erase = true);
	void informConnexionDeleted(QtConnexion *connexion);

	void deleteAllConnexions();

	/* Update the base connexion (redraw the connexion for which this port is base) */
	void updateConnexion(QPointF const &altTargetPos = QPointF(0.0f, 0.0f));

	/* Hide the port and move the endpoint of the connexion to the header of the node */
	void collapse();

	/* Make the port visible and restore the endpoint of the connexion */
	void expand();

	/* Observers */

	QVector<QtConnexion *> &getConnexions();
	const QVector<QtConnexion *> &getConnexions() const;

	QString const &getPortName() const;

	Alignment const &getAlignment() const;

	int getPortType() const;

	bool isPortOpen() const;
	bool isOutputPort() const;
	bool isConnected() const;

private:
	/* Set a port open or closed; if the port is closed, no connexion can be
	 * made or there is already a connexion */
	void setPortOpen(bool open);
};

bool is_connexion_allowed(QtPort *from, QtPort *to);
