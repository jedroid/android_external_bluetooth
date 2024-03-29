/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2011-2014  Intel Corporation
 *  Copyright (C) 2002-2010  Marcel Holtmann <marcel@holtmann.org>
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>

#include <bluetooth/bluetooth.h>

#include "src/shared/util.h"
#include "bt.h"
#include "packet.h"
#include "display.h"
#include "l2cap.h"
#include "uuid.h"
#include "keys.h"
#include "sdp.h"
#include "avctp.h"

/* ctype entries */
#define AVC_CTYPE_CONTROL		0x0
#define AVC_CTYPE_STATUS		0x1
#define AVC_CTYPE_SPECIFIC_INQUIRY	0x2
#define AVC_CTYPE_NOTIFY		0x3
#define AVC_CTYPE_GENERAL_INQUIRY	0x4
#define AVC_CTYPE_NOT_IMPLEMENTED	0x8
#define AVC_CTYPE_ACCEPTED		0x9
#define AVC_CTYPE_REJECTED		0xA
#define AVC_CTYPE_IN_TRANSITION		0xB
#define AVC_CTYPE_STABLE		0xC
#define AVC_CTYPE_CHANGED		0xD
#define AVC_CTYPE_INTERIM		0xF

/* subunit type */
#define AVC_SUBUNIT_MONITOR		0x00
#define AVC_SUBUNIT_AUDIO		0x01
#define AVC_SUBUNIT_PRINTER		0x02
#define AVC_SUBUNIT_DISC		0x03
#define AVC_SUBUNIT_TAPE		0x04
#define AVC_SUBUNIT_TURNER		0x05
#define AVC_SUBUNIT_CA			0x06
#define AVC_SUBUNIT_CAMERA		0x07
#define AVC_SUBUNIT_PANEL		0x09
#define AVC_SUBUNIT_BULLETIN_BOARD	0x0a
#define AVC_SUBUNIT_CAMERA_STORAGE	0x0b
#define AVC_SUBUNIT_VENDOR_UNIQUE	0x0c
#define AVC_SUBUNIT_EXTENDED		0x1e
#define AVC_SUBUNIT_UNIT		0x1f

/* opcodes */
#define AVC_OP_VENDORDEP		0x00
#define AVC_OP_UNITINFO			0x30
#define AVC_OP_SUBUNITINFO		0x31
#define AVC_OP_PASSTHROUGH		0x7c

/* notification events */
#define AVRCP_EVENT_PLAYBACK_STATUS_CHANGED		0x01
#define AVRCP_EVENT_TRACK_CHANGED			0x02
#define AVRCP_EVENT_TRACK_REACHED_END			0x03
#define AVRCP_EVENT_TRACK_REACHED_START			0x04
#define AVRCP_EVENT_PLAYBACK_POS_CHANGED		0x05
#define AVRCP_EVENT_BATT_STATUS_CHANGED			0x06
#define AVRCP_EVENT_SYSTEM_STATUS_CHANGED		0x07
#define AVRCP_EVENT_PLAYER_APPLICATION_SETTING_CHANGED	0x08
#define AVRCP_EVENT_NOW_PLAYING_CONTENT_CHANGED		0x09
#define AVRCP_EVENT_AVAILABLE_PLAYERS_CHANGED		0x0a
#define AVRCP_EVENT_ADDRESSED_PLAYER_CHANGED		0x0b
#define AVRCP_EVENT_UIDS_CHANGED			0x0c
#define AVRCP_EVENT_VOLUME_CHANGED			0x0d

/* error statuses */
#define AVRCP_STATUS_INVALID_COMMAND			0x00
#define AVRCP_STATUS_INVALID_PARAMETER			0x01
#define AVRCP_STATUS_NOT_FOUND				0x02
#define AVRCP_STATUS_INTERNAL_ERROR			0x03
#define AVRCP_STATUS_SUCCESS				0x04
#define AVRCP_STATUS_UID_CHANGED			0x05
#define AVRCP_STATUS_INVALID_DIRECTION			0x07
#define AVRCP_STATUS_NOT_DIRECTORY			0x08
#define AVRCP_STATUS_DOES_NOT_EXIST			0x09
#define AVRCP_STATUS_INVALID_SCOPE			0x0a
#define AVRCP_STATUS_OUT_OF_BOUNDS			0x0b
#define AVRCP_STATUS_IS_DIRECTORY			0x0c
#define AVRCP_STATUS_MEDIA_IN_USE			0x0d
#define AVRCP_STATUS_NOW_PLAYING_LIST_FULL		0x0e
#define AVRCP_STATUS_SEARCH_NOT_SUPPORTED		0x0f
#define AVRCP_STATUS_SEARCH_IN_PROGRESS			0x10
#define AVRCP_STATUS_INVALID_PLAYER_ID			0x11
#define AVRCP_STATUS_PLAYER_NOT_BROWSABLE		0x12
#define AVRCP_STATUS_PLAYER_NOT_ADDRESSED		0x13
#define AVRCP_STATUS_NO_VALID_SEARCH_RESULTS		0x14
#define AVRCP_STATUS_NO_AVAILABLE_PLAYERS		0x15
#define AVRCP_STATUS_ADDRESSED_PLAYER_CHANGED		0x16

/* pdu ids */
#define AVRCP_GET_CAPABILITIES		0x10
#define AVRCP_LIST_PLAYER_ATTRIBUTES	0x11
#define AVRCP_LIST_PLAYER_VALUES	0x12
#define AVRCP_GET_CURRENT_PLAYER_VALUE	0x13
#define AVRCP_SET_PLAYER_VALUE		0x14
#define AVRCP_GET_PLAYER_ATTRIBUTE_TEXT	0x15
#define AVRCP_GET_PLAYER_VALUE_TEXT	0x16
#define AVRCP_DISPLAYABLE_CHARSET	0x17
#define AVRCP_CT_BATTERY_STATUS		0x18
#define AVRCP_GET_ELEMENT_ATTRIBUTES	0x20
#define AVRCP_GET_PLAY_STATUS		0x30
#define AVRCP_REGISTER_NOTIFICATION	0x31
#define AVRCP_REQUEST_CONTINUING	0x40
#define AVRCP_ABORT_CONTINUING		0x41
#define AVRCP_SET_ABSOLUTE_VOLUME	0x50
#define AVRCP_SET_ADDRESSED_PLAYER	0x60
#define AVRCP_SET_BROWSED_PLAYER	0x70
#define AVRCP_GET_FOLDER_ITEMS		0x71
#define AVRCP_CHANGE_PATH		0x72
#define AVRCP_GET_ITEM_ATTRIBUTES	0x73
#define AVRCP_PLAY_ITEM			0x74
#define AVRCP_SEARCH			0x80
#define AVRCP_ADD_TO_NOW_PLAYING	0x90
#define AVRCP_GENERAL_REJECT		0xA0

/* Packet types */
#define AVRCP_PACKET_TYPE_SINGLE	0x00
#define AVRCP_PACKET_TYPE_START		0x01
#define AVRCP_PACKET_TYPE_CONTINUING	0x02
#define AVRCP_PACKET_TYPE_END		0x03

/* player attributes */
#define AVRCP_ATTRIBUTE_ILEGAL		0x00
#define AVRCP_ATTRIBUTE_EQUALIZER	0x01
#define AVRCP_ATTRIBUTE_REPEAT_MODE	0x02
#define AVRCP_ATTRIBUTE_SHUFFLE		0x03
#define AVRCP_ATTRIBUTE_SCAN		0x04

/* media attributes */
#define AVRCP_MEDIA_ATTRIBUTE_ILLEGAL	0x00
#define AVRCP_MEDIA_ATTRIBUTE_TITLE	0x01
#define AVRCP_MEDIA_ATTRIBUTE_ARTIST	0x02
#define AVRCP_MEDIA_ATTRIBUTE_ALBUM	0x03
#define AVRCP_MEDIA_ATTRIBUTE_TRACK	0x04
#define AVRCP_MEDIA_ATTRIBUTE_TOTAL	0x05
#define AVRCP_MEDIA_ATTRIBUTE_GENRE	0x06
#define AVRCP_MEDIA_ATTRIBUTE_DURATION	0x07

struct avctp_frame {
	uint8_t hdr;
	uint8_t pt;
	uint16_t pid;
	struct l2cap_frame l2cap_frame;
};

static struct avrcp_continuing {
	uint16_t num;
	uint16_t size;
} avrcp_continuing;

static const char *ctype2str(uint8_t ctype)
{
	switch (ctype & 0x0f) {
	case AVC_CTYPE_CONTROL:
		return "Control";
	case AVC_CTYPE_STATUS:
		return "Status";
	case AVC_CTYPE_SPECIFIC_INQUIRY:
		return "Specific Inquiry";
	case AVC_CTYPE_NOTIFY:
		return "Notify";
	case AVC_CTYPE_GENERAL_INQUIRY:
		return "General Inquiry";
	case AVC_CTYPE_NOT_IMPLEMENTED:
		return "Not Implemented";
	case AVC_CTYPE_ACCEPTED:
		return "Accepted";
	case AVC_CTYPE_REJECTED:
		return "Rejected";
	case AVC_CTYPE_IN_TRANSITION:
		return "In Transition";
	case AVC_CTYPE_STABLE:
		return "Stable";
	case AVC_CTYPE_CHANGED:
		return "Changed";
	case AVC_CTYPE_INTERIM:
		return "Interim";
	default:
		return "Unknown";
	}
}

static const char *subunit2str(uint8_t subunit)
{
	switch (subunit) {
	case AVC_SUBUNIT_MONITOR:
		return "Monitor";
	case AVC_SUBUNIT_AUDIO:
		return "Audio";
	case AVC_SUBUNIT_PRINTER:
		return "Printer";
	case AVC_SUBUNIT_DISC:
		return "Disc";
	case AVC_SUBUNIT_TAPE:
		return "Tape";
	case AVC_SUBUNIT_TURNER:
		return "Turner";
	case AVC_SUBUNIT_CA:
		return "CA";
	case AVC_SUBUNIT_CAMERA:
		return "Camera";
	case AVC_SUBUNIT_PANEL:
		return "Panel";
	case AVC_SUBUNIT_BULLETIN_BOARD:
		return "Bulleting Board";
	case AVC_SUBUNIT_CAMERA_STORAGE:
		return "Camera Storage";
	case AVC_SUBUNIT_VENDOR_UNIQUE:
		return "Vendor Unique";
	case AVC_SUBUNIT_EXTENDED:
		return "Extended to next byte";
	case AVC_SUBUNIT_UNIT:
		return "Unit";
	default:
		return "Reserved";
	}
}

static const char *opcode2str(uint8_t opcode)
{
	switch (opcode) {
	case AVC_OP_VENDORDEP:
		return "Vendor Dependent";
	case AVC_OP_UNITINFO:
		return "Unit Info";
	case AVC_OP_SUBUNITINFO:
		return "Subunit Info";
	case AVC_OP_PASSTHROUGH:
		return "Passthrough";
	default:
		return "Unknown";
	}
}

static char *cap2str(uint8_t cap)
{
	switch (cap) {
	case 0x2:
		return "CompanyID";
	case 0x3:
		return "EventsID";
	default:
		return "Unknown";
	}
}

static char *event2str(uint8_t event)
{
	switch (event) {
	case AVRCP_EVENT_PLAYBACK_STATUS_CHANGED:
		return "EVENT_PLAYBACK_STATUS_CHANGED";
	case AVRCP_EVENT_TRACK_CHANGED:
		return "EVENT_TRACK_CHANGED";
	case AVRCP_EVENT_TRACK_REACHED_END:
		return "EVENT_TRACK_REACHED_END";
	case AVRCP_EVENT_TRACK_REACHED_START:
		return "EVENT_TRACK_REACHED_START";
	case AVRCP_EVENT_PLAYBACK_POS_CHANGED:
		return "EVENT_PLAYBACK_POS_CHANGED";
	case AVRCP_EVENT_BATT_STATUS_CHANGED:
		return "EVENT_BATT_STATUS_CHANGED";
	case AVRCP_EVENT_SYSTEM_STATUS_CHANGED:
		return "EVENT_SYSTEM_STATUS_CHANGED";
	case AVRCP_EVENT_PLAYER_APPLICATION_SETTING_CHANGED:
		return "EVENT_PLAYER_APPLICATION_SETTING_CHANGED";
	case AVRCP_EVENT_NOW_PLAYING_CONTENT_CHANGED:
		return "EVENT_NOW_PLAYING_CONTENT_CHANGED";
	case AVRCP_EVENT_AVAILABLE_PLAYERS_CHANGED:
		return "EVENT_AVAILABLE_PLAYERS_CHANGED";
	case AVRCP_EVENT_ADDRESSED_PLAYER_CHANGED:
		return "EVENT_ADDRESSED_PLAYER_CHANGED";
	case AVRCP_EVENT_UIDS_CHANGED:
		return "EVENT_UIDS_CHANGED";
	case AVRCP_EVENT_VOLUME_CHANGED:
		return "EVENT_VOLUME_CHANGED";
	default:
		return "Reserved";
	}
}

static const char *error2str(uint8_t status)
{
	switch (status) {
	case AVRCP_STATUS_INVALID_COMMAND:
		return "Invalid Command";
	case AVRCP_STATUS_INVALID_PARAMETER:
		return "Invalid Parameter";
	case AVRCP_STATUS_NOT_FOUND:
		return "Not Found";
	case AVRCP_STATUS_INTERNAL_ERROR:
		return "Internal Error";
	case AVRCP_STATUS_SUCCESS:
		return "Success";
	case AVRCP_STATUS_UID_CHANGED:
		return "UID Changed";
	case AVRCP_STATUS_INVALID_DIRECTION:
		return "Invalid Direction";
	case AVRCP_STATUS_NOT_DIRECTORY:
		return "Not a Directory";
	case AVRCP_STATUS_DOES_NOT_EXIST:
		return "Does Not Exist";
	case AVRCP_STATUS_INVALID_SCOPE:
		return "Invalid Scope";
	case AVRCP_STATUS_OUT_OF_BOUNDS:
		return "Range Out of Bonds";
	case AVRCP_STATUS_MEDIA_IN_USE:
		return "Media in Use";
	case AVRCP_STATUS_IS_DIRECTORY:
		return "UID is a Directory";
	case AVRCP_STATUS_NOW_PLAYING_LIST_FULL:
		return "Now Playing List Full";
	case AVRCP_STATUS_SEARCH_NOT_SUPPORTED:
		return "Seach Not Supported";
	case AVRCP_STATUS_SEARCH_IN_PROGRESS:
		return "Search in Progress";
	case AVRCP_STATUS_INVALID_PLAYER_ID:
		return "Invalid Player ID";
	case AVRCP_STATUS_PLAYER_NOT_BROWSABLE:
		return "Player Not Browsable";
	case AVRCP_STATUS_PLAYER_NOT_ADDRESSED:
		return "Player Not Addressed";
	case AVRCP_STATUS_NO_VALID_SEARCH_RESULTS:
		return "No Valid Search Result";
	case AVRCP_STATUS_NO_AVAILABLE_PLAYERS:
		return "No Available Players";
	case AVRCP_STATUS_ADDRESSED_PLAYER_CHANGED:
		return "Addressed Player Changed";
	default:
		return "Unknown";
	}
}

static const char *pdu2str(uint8_t pduid)
{
	switch (pduid) {
	case AVRCP_GET_CAPABILITIES:
		return "GetCapabilities";
	case AVRCP_LIST_PLAYER_ATTRIBUTES:
		return "ListPlayerApplicationSettingAttributes";
	case AVRCP_LIST_PLAYER_VALUES:
		return "ListPlayerApplicationSettingValues";
	case AVRCP_GET_CURRENT_PLAYER_VALUE:
		return "GetCurrentPlayerApplicationSettingValue";
	case AVRCP_SET_PLAYER_VALUE:
		return "SetPlayerApplicationSettingValue";
	case AVRCP_GET_PLAYER_ATTRIBUTE_TEXT:
		return "GetPlayerApplicationSettingAttributeText";
	case AVRCP_GET_PLAYER_VALUE_TEXT:
		return "GetPlayerApplicationSettingValueText";
	case AVRCP_DISPLAYABLE_CHARSET:
		return "InformDisplayableCharacterSet";
	case AVRCP_CT_BATTERY_STATUS:
		return "InformBatteryStatusOfCT";
	case AVRCP_GET_ELEMENT_ATTRIBUTES:
		return "GetElementAttributes";
	case AVRCP_GET_PLAY_STATUS:
		return "GetPlayStatus";
	case AVRCP_REGISTER_NOTIFICATION:
		return "RegisterNotification";
	case AVRCP_REQUEST_CONTINUING:
		return "RequestContinuingResponse";
	case AVRCP_ABORT_CONTINUING:
		return "AbortContinuingResponse";
	case AVRCP_SET_ABSOLUTE_VOLUME:
		return "SetAbsoluteVolume";
	case AVRCP_SET_ADDRESSED_PLAYER:
		return "SetAddressedPlayer";
	case AVRCP_SET_BROWSED_PLAYER:
		return "SetBrowsedPlayer";
	case AVRCP_GET_FOLDER_ITEMS:
		return "GetFolderItems";
	case AVRCP_CHANGE_PATH:
		return "ChangePath";
	case AVRCP_GET_ITEM_ATTRIBUTES:
		return "GetItemAttributes";
	case AVRCP_PLAY_ITEM:
		return "PlayItem";
	case AVRCP_SEARCH:
		return "Search";
	case AVRCP_ADD_TO_NOW_PLAYING:
		return "AddToNowPlaying";
	case AVRCP_GENERAL_REJECT:
		return "GeneralReject";
	default:
		return "Unknown";
	}
}

static const char *pt2str(uint8_t pt)
{
	switch (pt) {
	case AVRCP_PACKET_TYPE_SINGLE:
		return "Single";
	case AVRCP_PACKET_TYPE_START:
		return "Start";
	case AVRCP_PACKET_TYPE_CONTINUING:
		return "Continuing";
	case AVRCP_PACKET_TYPE_END:
		return "End";
	default:
		return "Unknown";
	}
}

static const char *attr2str(uint8_t attr)
{
	switch (attr) {
	case AVRCP_ATTRIBUTE_ILEGAL:
		return "Illegal";
	case AVRCP_ATTRIBUTE_EQUALIZER:
		return "Equalizer ON/OFF Status";
	case AVRCP_ATTRIBUTE_REPEAT_MODE:
		return "Repeat Mode Status";
	case AVRCP_ATTRIBUTE_SHUFFLE:
		return "Shuffle ON/OFF Status";
	case AVRCP_ATTRIBUTE_SCAN:
		return "Scan ON/OFF Status";
	default:
		return "Unknown";
	}
}

static const char *value2str(uint8_t attr, uint8_t value)
{
	switch (attr) {
	case AVRCP_ATTRIBUTE_ILEGAL:
		return "Illegal";
	case AVRCP_ATTRIBUTE_EQUALIZER:
		switch (value) {
		case 0x01:
			return "OFF";
		case 0x02:
			return "ON";
		default:
			return "Reserved";
		}
	case AVRCP_ATTRIBUTE_REPEAT_MODE:
		switch (value) {
		case 0x01:
			return "OFF";
		case 0x02:
			return "Single Track Repeat";
		case 0x03:
			return "All Track Repeat";
		case 0x04:
			return "Group Repeat";
		default:
			return "Reserved";
		}
	case AVRCP_ATTRIBUTE_SHUFFLE:
		switch (value) {
		case 0x01:
			return "OFF";
		case 0x02:
			return "All Track Suffle";
		case 0x03:
			return "Group Suffle";
		default:
			return "Reserved";
		}
	case AVRCP_ATTRIBUTE_SCAN:
		switch (value) {
		case 0x01:
			return "OFF";
		case 0x02:
			return "All Track Scan";
		case 0x03:
			return "Group Scan";
		default:
			return "Reserved";
		}
	default:
		return "Unknown";
	}
}

static const char *charset2str(uint16_t charset)
{
	switch (charset) {
	case 1:
	case 2:
		return "Reserved";
	case 3:
		return "ASCII";
	case 4:
		return "ISO_8859-1";
	case 5:
		return "ISO_8859-2";
	case 6:
		return "ISO_8859-3";
	case 7:
		return "ISO_8859-4";
	case 8:
		return "ISO_8859-5";
	case 9:
		return "ISO_8859-6";
	case 10:
		return "ISO_8859-7";
	case 11:
		return "ISO_8859-8";
	case 12:
		return "ISO_8859-9";
	case 106:
		return "UTF-8";
	default:
		return "Unknown";
	}
}

static const char *mediattr2str(uint32_t attr)
{
	switch (attr) {
	case AVRCP_MEDIA_ATTRIBUTE_ILLEGAL:
		return "Illegal";
	case AVRCP_MEDIA_ATTRIBUTE_TITLE:
		return "Title";
	case AVRCP_MEDIA_ATTRIBUTE_ARTIST:
		return "Artist";
	case AVRCP_MEDIA_ATTRIBUTE_ALBUM:
		return "Album";
	case AVRCP_MEDIA_ATTRIBUTE_TRACK:
		return "Track";
	case AVRCP_MEDIA_ATTRIBUTE_TOTAL:
		return "Track Total";
	case AVRCP_MEDIA_ATTRIBUTE_GENRE:
		return "Genre";
	case AVRCP_MEDIA_ATTRIBUTE_DURATION:
		return "Track duration";
	default:
		return "Reserved";
	}
}

static bool avrcp_passthrough_packet(struct avctp_frame *avctp_frame)
{
	struct l2cap_frame *frame = &avctp_frame->l2cap_frame;

	packet_hexdump(frame->data, frame->size);
	return true;
}

static bool avrcp_get_capabilities(struct avctp_frame *avctp_frame,
					uint8_t ctype, uint8_t len,
					uint8_t indent)
{
	struct l2cap_frame *frame = &avctp_frame->l2cap_frame;
	uint8_t cap, count;
	int i;

	if (!l2cap_frame_get_u8(frame, &cap))
		return false;

	print_field("%*cCapabilityID: 0x%02x (%s)", (indent - 8), ' ', cap,
								cap2str(cap));

	if (len == 1)
		return true;

	if (!l2cap_frame_get_u8(frame, &count))
		return false;

	print_field("%*cCapabilityCount: 0x%02x", (indent - 8), ' ', count);

	switch (cap) {
	case 0x2:
		for (; count > 0; count--) {
			uint8_t company[3];

			if (!l2cap_frame_get_u8(frame, &company[0]) ||
				!l2cap_frame_get_u8(frame, &company[1]) ||
				!l2cap_frame_get_u8(frame, &company[2]))
				return false;

			print_field("%*c%s: 0x%02x%02x%02x", (indent - 8), ' ',
					cap2str(cap), company[0], company[1],
					company[2]);
		}
		break;
	case 0x3:
		for (i = 0; count > 0; count--, i++) {
			uint8_t event;

			if (!l2cap_frame_get_u8(frame, &event))
				return false;

			print_field("%*c%s: 0x%02x (%s)", (indent - 8), ' ',
					cap2str(cap), event, event2str(event));
		}
		break;
	default:
		packet_hexdump(frame->data, frame->size);
	}

	return true;
}

static bool avrcp_list_player_attributes(struct avctp_frame *avctp_frame,
						uint8_t ctype, uint8_t len,
						uint8_t indent)
{
	struct l2cap_frame *frame = &avctp_frame->l2cap_frame;
	uint8_t num;
	int i;

	if (len == 0)
		return true;

	if (!l2cap_frame_get_u8(frame, &num))
		return false;

	print_field("%*cAttributeCount: 0x%02x", (indent - 8), ' ', num);

	for (i = 0; num > 0; num--, i++) {
		uint8_t attr;

		if (!l2cap_frame_get_u8(frame, &attr))
			return false;

		print_field("%*cAttributeID: 0x%02x (%s)", (indent - 8), ' ',
							attr, attr2str(attr));
	}

	return true;
}

static bool avrcp_list_player_values(struct avctp_frame *avctp_frame,
					uint8_t ctype, uint8_t len,
					uint8_t indent)
{
	struct l2cap_frame *frame = &avctp_frame->l2cap_frame;
	static uint8_t attr = 0;
	uint8_t num;

	if (ctype > AVC_CTYPE_GENERAL_INQUIRY)
		goto response;

	if (!l2cap_frame_get_u8(frame, &attr))
		return false;

	print_field("%*cAttributeID: 0x%02x (%s)", (indent - 8), ' ',
						attr, attr2str(attr));

	return true;

response:
	if (!l2cap_frame_get_u8(frame, &num))
		return false;

	print_field("%*cValueCount: 0x%02x", (indent - 8), ' ', num);

	for (; num > 0; num--) {
		uint8_t value;

		if (!l2cap_frame_get_u8(frame, &value))
			return false;

		print_field("%*cValueID: 0x%02x (%s)", (indent - 8),
					' ', value, value2str(attr, value));
	}

	return true;
}

static bool avrcp_get_current_player_value(struct avctp_frame *avctp_frame,
						uint8_t ctype, uint8_t len,
						uint8_t indent)
{
	struct l2cap_frame *frame = &avctp_frame->l2cap_frame;
	uint8_t num;

	if (!l2cap_frame_get_u8(frame, &num))
		return false;

	if (ctype > AVC_CTYPE_GENERAL_INQUIRY)
		goto response;

	print_field("%*cAttributeCount: 0x%02x", (indent - 8), ' ', num);

	for (; num > 0; num--) {
		uint8_t attr;

		if (!l2cap_frame_get_u8(frame, &attr))
			return false;

		print_field("%*cAttributeID: 0x%02x (%s)", (indent - 8),
						' ', attr, attr2str(attr));
	}

	return true;

response:
	print_field("%*cValueCount: 0x%02x", (indent - 8), ' ', num);

	for (; num > 0; num--) {
		uint8_t attr, value;

		if (!l2cap_frame_get_u8(frame, &attr))
			return false;

		print_field("%*cAttributeID: 0x%02x (%s)", (indent - 8),
						' ', attr, attr2str(attr));

		if (!l2cap_frame_get_u8(frame, &value))
			return false;

		print_field("%*cValueID: 0x%02x (%s)", (indent - 8),
					' ', value, value2str(attr, value));
	}

	return true;
}

static bool avrcp_set_player_value(struct avctp_frame *avctp_frame,
					uint8_t ctype, uint8_t len,
					uint8_t indent)
{
	struct l2cap_frame *frame = &avctp_frame->l2cap_frame;
	uint8_t num;

	if (ctype > AVC_CTYPE_GENERAL_INQUIRY)
		return true;

	if (!l2cap_frame_get_u8(frame, &num))
		return false;

	print_field("%*cAttributeCount: 0x%02x", (indent - 8), ' ', num);

	for (; num > 0; num--) {
		uint8_t attr, value;

		if (!l2cap_frame_get_u8(frame, &attr))
			return false;

		print_field("%*cAttributeID: 0x%02x (%s)", (indent - 8), ' ',
							attr, attr2str(attr));

		if (!l2cap_frame_get_u8(frame, &value))
			return false;

		print_field("%*cValueID: 0x%02x (%s)", (indent - 8), ' ',
						value, value2str(attr, value));
	}

	return true;
}

static bool avrcp_get_player_attribute_text(struct avctp_frame *avctp_frame,
						uint8_t ctype, uint8_t len,
						uint8_t indent)
{
	struct l2cap_frame *frame = &avctp_frame->l2cap_frame;
	uint8_t num;

	if (!l2cap_frame_get_u8(frame, &num))
		return false;

	print_field("%*cAttributeCount: 0x%02x", (indent - 8), ' ', num);

	if (ctype > AVC_CTYPE_GENERAL_INQUIRY)
		goto response;

	for (; num > 0; num--) {
		uint8_t attr;

		if (!l2cap_frame_get_u8(frame, &attr))
			return false;

		print_field("%*cAttributeID: 0x%02x (%s)", (indent - 8),
						' ', attr, attr2str(attr));
	}

	return true;

response:
	for (; num > 0; num--) {
		uint8_t attr, len;
		uint16_t charset;

		if (!l2cap_frame_get_u8(frame, &attr))
			return false;

		print_field("%*cAttributeID: 0x%02x (%s)", (indent - 8),
						' ', attr, attr2str(attr));

		if (!l2cap_frame_get_be16(frame, &charset))
			return false;

		print_field("%*cCharsetID: 0x%04x (%s)", (indent - 8),
					' ', charset, charset2str(charset));

		if (!l2cap_frame_get_u8(frame, &len))
			return false;

		print_field("%*cStringLength: 0x%02x", (indent - 8), ' ', len);

		printf("String: ");
		for (; len > 0; len--) {
			uint8_t c;

			if (!l2cap_frame_get_u8(frame, &c))
				return false;

			printf("%1c", isprint(c) ? c : '.');
		}
		printf("\n");
	}

	return true;
}

static bool avrcp_get_player_value_text(struct avctp_frame *avctp_frame,
					uint8_t ctype, uint8_t len,
					uint8_t indent)
{
	struct l2cap_frame *frame = &avctp_frame->l2cap_frame;
	static uint8_t attr = 0;
	uint8_t num;

	if (ctype > AVC_CTYPE_GENERAL_INQUIRY)
		goto response;

	if (!l2cap_frame_get_u8(frame, &attr))
		return false;

	print_field("%*cAttributeID: 0x%02x (%s)", (indent - 8), ' ',
						attr, attr2str(attr));

	if (!l2cap_frame_get_u8(frame, &num))
		return false;

	print_field("%*cValueCount: 0x%02x", (indent - 8), ' ', num);

	for (; num > 0; num--) {
		uint8_t value;

		if (!l2cap_frame_get_u8(frame, &value))
			return false;

		print_field("%*cValueID: 0x%02x (%s)", (indent - 8),
				' ', value, value2str(attr, value));
	}

	return true;

response:
	if (!l2cap_frame_get_u8(frame, &num))
		return false;

	print_field("%*cValueCount: 0x%02x", (indent - 8), ' ', num);

	for (; num > 0; num--) {
		uint8_t value, len;
		uint16_t charset;

		if (!l2cap_frame_get_u8(frame, &value))
			return false;

		print_field("%*cValueID: 0x%02x (%s)", (indent - 8), ' ',
						value, value2str(attr, value));

		if (!l2cap_frame_get_be16(frame, &charset))
			return false;

		print_field("%*cCharsetIDID: 0x%02x (%s)", (indent - 8), ' ',
						charset, charset2str(charset));

		if (!l2cap_frame_get_u8(frame, &len))
			return false;

		print_field("%*cStringLength: 0x%02x", (indent - 8), ' ', len);

		printf("String: ");
		for (; len > 0; len--) {
			uint8_t c;

			if (!l2cap_frame_get_u8(frame, &c))
				return false;

			printf("%1c", isprint(c) ? c : '.');
		}
		printf("\n");
	}

	return true;
}

static bool avrcp_displayable_charset(struct avctp_frame *avctp_frame,
					uint8_t ctype, uint8_t len,
					uint8_t indent)
{
	struct l2cap_frame *frame = &avctp_frame->l2cap_frame;
	uint8_t num;

	if (ctype > AVC_CTYPE_GENERAL_INQUIRY)
		return true;

	if (!l2cap_frame_get_u8(frame, &num))
		return false;

	print_field("%*cCharsetCount: 0x%02x", (indent - 8), ' ', num);

	for (; num > 0; num--) {
		uint16_t charset;

		if (!l2cap_frame_get_be16(frame, &charset))
			return false;

		print_field("%*cCharsetID: 0x%04x (%s)", (indent - 8),
					' ', charset, charset2str(charset));
	}

	return true;
}

static bool avrcp_get_element_attributes(struct avctp_frame *avctp_frame,
						uint8_t ctype, uint8_t len,
						uint8_t indent)
{
	struct l2cap_frame *frame = &avctp_frame->l2cap_frame;
	uint64_t id;
	uint8_t num;

	if (ctype > AVC_CTYPE_GENERAL_INQUIRY)
		goto response;

	if (!l2cap_frame_get_be64(frame, &id))
		return false;

	print_field("%*cIdentifier: 0x%jx (%s)", (indent - 8), ' ',
					id, id ? "Reserved" : "PLAYING");

	if (!l2cap_frame_get_u8(frame, &num))
		return false;

	print_field("%*cAttributeCount: 0x%02x", (indent - 8), ' ', num);

	for (; num > 0; num--) {
		uint32_t attr;

		if (!l2cap_frame_get_be32(frame, &attr))
			return false;

		print_field("%*cAttribute: 0x%08x (%s)", (indent - 8),
					' ', attr, mediattr2str(attr));
	}

	return true;

response:
	switch (avctp_frame->pt) {
	case AVRCP_PACKET_TYPE_SINGLE:
	case AVRCP_PACKET_TYPE_START:
		if (!l2cap_frame_get_u8(frame, &num))
			return false;

		avrcp_continuing.num = num;
		print_field("%*cAttributeCount: 0x%02x", (indent - 8),
								' ', num);
		len--;
		break;
	case AVRCP_PACKET_TYPE_CONTINUING:
	case AVRCP_PACKET_TYPE_END:
		num = avrcp_continuing.num;

		if (avrcp_continuing.size > 0) {
			uint16_t size;

			if (avrcp_continuing.size > len) {
				size = len;
				avrcp_continuing.size -= len;
			} else {
				size = avrcp_continuing.size;
				avrcp_continuing.size = 0;
			}

			printf("ContinuingAttributeValue: ");
			for (; size > 0; size--) {
				uint8_t c;

				if (!l2cap_frame_get_u8(frame, &c))
					goto failed;

				printf("%1c", isprint(c) ? c : '.');
			}
			printf("\n");

			len -= size;
		}
		break;
	default:
		goto failed;
	}

	while (num > 0 && len > 0) {
		uint32_t attr;
		uint16_t charset, attrlen;

		if (!l2cap_frame_get_be32(frame, &attr))
			goto failed;

		print_field("%*cAttribute: 0x%08x (%s)", (indent - 8),
						' ', attr, mediattr2str(attr));

		if (!l2cap_frame_get_be16(frame, &charset))
			goto failed;

		print_field("%*cCharsetID: 0x%04x (%s)", (indent - 8),
				' ', charset, charset2str(charset));

		if (!l2cap_frame_get_be16(frame, &attrlen))
			goto failed;

		print_field("%*cAttributeValueLength: 0x%04x",
						(indent - 8), ' ', attrlen);

		len -= sizeof(attr) + sizeof(charset) + sizeof(attrlen);
		num--;

		print_field("%*cAttributeValue: ", (indent - 8), ' ');
		for (; attrlen > 0 && len > 0; attrlen--, len--) {
			uint8_t c;

			if (!l2cap_frame_get_u8(frame, &c))
				goto failed;

			printf("%1c", isprint(c) ? c : '.');
		}

		if (attrlen > 0)
			avrcp_continuing.size = attrlen;
	}

	avrcp_continuing.num = num;
	return true;

failed:
	avrcp_continuing.num = 0;
	avrcp_continuing.size = 0;
	return false;
}

struct avrcp_ctrl_pdu_data {
	uint8_t pduid;
	bool (*func) (struct avctp_frame *avctp_frame, uint8_t ctype,
						uint8_t len, uint8_t indent);
};

static const struct avrcp_ctrl_pdu_data avrcp_ctrl_pdu_table[] = {
	{ 0x10, avrcp_get_capabilities			},
	{ 0x11, avrcp_list_player_attributes		},
	{ 0x12, avrcp_list_player_values		},
	{ 0x13, avrcp_get_current_player_value		},
	{ 0x14, avrcp_set_player_value			},
	{ 0x15, avrcp_get_player_attribute_text		},
	{ 0x16, avrcp_get_player_value_text		},
	{ 0x17, avrcp_displayable_charset		},
	{ 0x20, avrcp_get_element_attributes		},
	{ }
};

static bool avrcp_rejected_packet(struct l2cap_frame *frame, uint8_t indent)
{
	uint8_t status;

	if (!l2cap_frame_get_u8(frame, &status))
		return false;

	print_field("%*cError: 0x%02x (%s)", (indent - 8), ' ', status,
							error2str(status));

	return true;
}

static bool avrcp_pdu_packet(struct avctp_frame *avctp_frame, uint8_t ctype,
								uint8_t indent)
{
	struct l2cap_frame *frame = &avctp_frame->l2cap_frame;
	uint8_t pduid;
	uint16_t len;
	int i;
	const struct avrcp_ctrl_pdu_data *ctrl_pdu_data = NULL;

	if (!l2cap_frame_get_u8(frame, &pduid))
		return false;

	if (!l2cap_frame_get_u8(frame, &avctp_frame->pt))
		return false;

	if (!l2cap_frame_get_be16(frame, &len))
		return false;

	print_indent(indent, COLOR_OFF, "AVRCP: ", pdu2str(pduid), COLOR_OFF,
			" pt %s len 0x%04x", pt2str(avctp_frame->pt), len);

	if (frame->size != len)
		return false;

	if (ctype == 0xA)
		return avrcp_rejected_packet(frame, indent + 2);

	for (i = 0; avrcp_ctrl_pdu_table[i].func; i++) {
		if (avrcp_ctrl_pdu_table[i].pduid == pduid) {
			ctrl_pdu_data = &avrcp_ctrl_pdu_table[i];
			break;
		}
	}

	if (!ctrl_pdu_data || !ctrl_pdu_data->func) {
		packet_hexdump(frame->data, frame->size);
		return true;
	}

	return ctrl_pdu_data->func(avctp_frame, ctype, len, indent + 2);
}

static bool avrcp_control_packet(struct avctp_frame *avctp_frame)
{
	struct l2cap_frame *frame = &avctp_frame->l2cap_frame;

	uint8_t ctype, address, subunit, opcode, company[3], indent = 2;

	if (!l2cap_frame_get_u8(frame, &ctype) ||
				!l2cap_frame_get_u8(frame, &address) ||
				!l2cap_frame_get_u8(frame, &opcode))
		return false;

	print_field("AV/C: %s: address 0x%02x opcode 0x%02x",
				ctype2str(ctype), address, opcode);

	subunit = address >> 3;

	print_field("%*cSubunit: %s", indent, ' ', subunit2str(subunit));

	print_field("%*cOpcode: %s", indent, ' ', opcode2str(opcode));

	/* Skip non-panel subunit packets */
	if (subunit != 0x09) {
		packet_hexdump(frame->data, frame->size);
		return true;
	}

	/* Not implemented should not contain any operand */
	if (ctype == 0x8) {
		packet_hexdump(frame->data, frame->size);
		return true;
	}

	switch (opcode) {
	case 0x7c:
		return avrcp_passthrough_packet(avctp_frame);
	case 0x00:
		if (!l2cap_frame_get_u8(frame, &company[0]) ||
				!l2cap_frame_get_u8(frame, &company[1]) ||
				!l2cap_frame_get_u8(frame, &company[2]))
			return false;

		print_field("%*cCompany ID: 0x%02x%02x%02x", indent, ' ',
					company[0], company[1], company[2]);

		return avrcp_pdu_packet(avctp_frame, ctype, 10);
	default:
		packet_hexdump(frame->data, frame->size);
		return true;
	}
}

static bool avrcp_browsing_packet(struct avctp_frame *avctp_frame)
{
	struct l2cap_frame *frame = &avctp_frame->l2cap_frame;

	packet_hexdump(frame->data, frame->size);
	return true;
}

static void avrcp_packet(struct avctp_frame *avctp_frame)
{
	struct l2cap_frame *frame = &avctp_frame->l2cap_frame;
	bool ret;

	switch (frame->psm) {
	case 0x17:
		ret = avrcp_control_packet(avctp_frame);
		break;
	case 0x1B:
		ret = avrcp_browsing_packet(avctp_frame);
		break;
	default:
		packet_hexdump(frame->data, frame->size);
		return;
	}

	if (!ret) {
		print_text(COLOR_ERROR, "PDU malformed");
		packet_hexdump(frame->data, frame->size);
	}
}

void avctp_packet(const struct l2cap_frame *frame)
{
	struct l2cap_frame *l2cap_frame;
	struct avctp_frame avctp_frame;
	const char *pdu_color;

	l2cap_frame_pull(&avctp_frame.l2cap_frame, frame, 0);

	l2cap_frame = &avctp_frame.l2cap_frame;

	if (!l2cap_frame_get_u8(l2cap_frame, &avctp_frame.hdr) ||
			!l2cap_frame_get_be16(l2cap_frame, &avctp_frame.pid)) {
		print_text(COLOR_ERROR, "frame too short");
		packet_hexdump(frame->data, frame->size);
		return;
	}

	if (frame->in)
		pdu_color = COLOR_MAGENTA;
	else
		pdu_color = COLOR_BLUE;

	print_indent(6, pdu_color, "AVCTP", "", COLOR_OFF,
				" %s: %s: type 0x%02x label %d PID 0x%04x",
				frame->psm == 23 ? "Control" : "Browsing",
				avctp_frame.hdr & 0x02 ? "Response" : "Command",
				avctp_frame.hdr & 0x0c, avctp_frame.hdr >> 4,
				avctp_frame.pid);

	if (avctp_frame.pid == 0x110e || avctp_frame.pid == 0x110c)
		avrcp_packet(&avctp_frame);
	else
		packet_hexdump(frame->data, frame->size);
}
