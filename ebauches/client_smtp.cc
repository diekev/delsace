// connect

/// reçoit réponse

S: 220 smtp.xxxx.xxxx SMTP Ready
C: HELO client
S: 250-smtp.xxxx.xxxx
S: 250-PIPELINING
S: 250 8BITMIME
C: MAIL FROM: <auteur@yyyy.yyyy>
S: 250 Sender ok
C: RCPT TO: <destinataire@xxxx.xxxx>
S: 250 Recipient ok.
C: DATA
S: 354 Enter mail, end with "." on a line by itself
C: Subject: Test
C:
C: Corps du texte
C: .
S: 250 Ok
C: QUIT
S: 221 Closing connection
Connection closed by foreign host.

ferme_connexion = false;

while (!ferme_connexion) {
	auto reponse = charge_reponse(m_prise);

	switch (reponse.status_smtp) {
		case 220:
			// succès connexion
			envoie("HELO client\r\n");
			etat = MAIL_FROM;
			break;
		case 221:
			//
			ferme_connexion = true;
			break;
		case 250:
			// confirmation de commande accepté
			if (etat == MAIL_FROM) {
				envoie("MAIL FROM: <auteur@yyyy.yyyy>\r\n");
				etat = RCPT_TO;
			}
			else if (etat == RCPT_TO) {
				envoie("RCPT TO: <destinataire@xxxx.xxxx>\r\n");
				etat = DATA;
				// a faire CC
			}
			else if (etat == DATA) {
				envoie("DATA\r\n");
			}
			else if (etat == FIN_DATA) {
				envoie("QUIT\r\n");
			}
			break;
		case 354:
			// reponse commande data
			assert(etat == DATA);
			// envoir mail. termine par "\r\n.\r\n";
			etat = FIN_DATA;
			break;
		case 421:
			// éhec temporaire
			break;
	}

}
