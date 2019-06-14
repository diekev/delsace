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

namespace dls {
namespace xml {

// WARNING: must match Document::_errorNames[]
enum XMLError {
	XML_SUCCESS = 0,
	XML_NO_ERROR = 0,
	XML_NO_ATTRIBUTE,
	XML_WRONG_ATTRIBUTE_TYPE,
	XML_ERROR_FILE_NOT_FOUND,
	XML_ERROR_FILE_COULD_NOT_BE_OPENED,
	XML_ERROR_FILE_READ_ERROR,
	XML_ERROR_ELEMENT_MISMATCH,
	XML_ERROR_PARSING_ELEMENT,
	XML_ERROR_PARSING_ATTRIBUTE,
	XML_ERROR_IDENTIFYING_TAG,
	XML_ERROR_PARSING_TEXT,
	XML_ERROR_PARSING_CDATA,
	XML_ERROR_PARSING_COMMENT,
	XML_ERROR_PARSING_DECLARATION,
	XML_ERROR_PARSING_UNKNOWN,
	XML_ERROR_EMPTY_DOCUMENT,
	XML_ERROR_MISMATCHED_ELEMENT,
	XML_ERROR_PARSING,
	XML_CAN_NOT_CONVERT_TEXT,
	XML_NO_TEXT_NODE,

	XML_ERROR_COUNT
};

}  /* namespace xml */
}  /* namespace dls */
