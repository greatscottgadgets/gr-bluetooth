/* packet-btbb.c
 * Routines for Bluetooth baseband dissection
 * Copyright 2009, Michael Ossmann <mike@ossmann.com>
 *
 * $Id$
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <epan/packet.h>
#include <epan/prefs.h>

/* function prototypes */
void proto_reg_handoff_btbb(void);

/* initialize the protocol and registered fields */
static int proto_btbb = -1;
static int hf_btbb_pkthdr = -1;
static int hf_btbb_ltaddr = -1;
static int hf_btbb_type = -1;
static int hf_btbb_flags = -1;
static int hf_btbb_flow = -1;
static int hf_btbb_arqn = -1;
static int hf_btbb_seqn = -1;
static int hf_btbb_hec = -1;
static int hf_btbb_payload = -1;

/* field values */
static const value_string packet_types[] = {
	/* generic names for unknown logical transport */
	{ 0x0, "NULL" },
	{ 0x1, "POLL" },
	{ 0x2, "FHS" },
	{ 0x3, "DM1" },
	{ 0x4, "DH1/2-DH1" },
	{ 0x5, "HV1" },
	{ 0x6, "HV2/2-EV3" },
	{ 0x7, "HV3/EV3/3-EV3" },
	{ 0x8, "DV/3-DH1" },
	{ 0x9, "AUX1" },
	{ 0xa, "DM3/2-DH3" },
	{ 0xb, "DH3/3-DH3" },
	{ 0xc, "EV4/2-EV5" },
	{ 0xd, "EV5/3-EV5" },
	{ 0xe, "DM5/2-DH5" },
	{ 0xf, "DH5/3-DH5" },
	{ 0, NULL }
};

/* initialize the subtree pointers */
static gint ett_btbb = -1;
static gint ett_btbb_pkthdr = -1;
static gint ett_btbb_flags = -1;
static gint ett_btbb_payload = -1;

/* packet header flags */
static const int *flag_fields[] = {
	&hf_btbb_flow,
	&hf_btbb_arqn,
	&hf_btbb_seqn,
	NULL
};

/* dissect a packet */
static int
dissect_btbb(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	proto_item *btbb_item, *pkthdr_item, *pld_item;
	proto_tree *btbb_tree, *pkthdr_tree;
	int offset;
	guint8 type;

	/* sanity check: long enough */
	if (tvb_length(tvb) < 3)
		/* Not enough bytes: look for a different dissector */
		return 0;

	/* maybe should verify HEC */

	/* make entries in protocol column and info column on summary display */
	if (check_col(pinfo->cinfo, COL_PROTOCOL))
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "Bluetooth");

	/* clear the info column first just in case of type fetching failure. */
	if (check_col(pinfo->cinfo, COL_INFO))
		col_clear(pinfo->cinfo, COL_INFO);

	type = (tvb_get_guint8(tvb, 0) >> 3) & 0x0f;
	if (check_col(pinfo->cinfo, COL_INFO))
		col_add_str(pinfo->cinfo, COL_INFO, match_strval(type, packet_types));

	/* see if we are being asked for details */
	if (tree) {

		/* create display subtree for the protocol */
		btbb_item = proto_tree_add_item(tree, proto_btbb, tvb, 0, -1, FALSE);
		btbb_tree = proto_item_add_subtree(btbb_item, ett_btbb);
		offset = 0;

		/* packet header */
		pkthdr_item = proto_tree_add_item(btbb_tree, hf_btbb_pkthdr, tvb, offset, 3, FALSE);
		pkthdr_tree = proto_item_add_subtree(pkthdr_item, ett_btbb_pkthdr);

		proto_tree_add_item(pkthdr_tree, hf_btbb_ltaddr, tvb, offset, 1, FALSE);
		proto_tree_add_item(pkthdr_tree, hf_btbb_type, tvb, offset, 1, FALSE);
		offset += 1;
		proto_tree_add_bitmask(pkthdr_tree, tvb, offset, hf_btbb_flags,
			ett_btbb_flags, flag_fields, FALSE);
		offset += 1;
		proto_tree_add_item(pkthdr_tree, hf_btbb_hec, tvb, offset, 1, FALSE);
		offset += 1;

		/* payload */
		switch (type) {
		case 0x0: /* NULL */
		case 0x1: /* POLL */
			break;
		case 0x2: /* FHS */
		case 0x3: /* DM1 */
		case 0x4: /* DH1/2-DH1 */
		case 0x5: /* HV1 */
		case 0x6: /* HV2/2-EV3 */
		case 0x7: /* HV3/EV3/3-EV3 */
		case 0x8: /* DV/3-DH1 */
		case 0x9: /* AUX1 */
		case 0xa: /* DM3/2-DH3 */
		case 0xb: /* DH3/3-DH3 */
		case 0xc: /* EV4/2-EV5 */
		case 0xd: /* EV5/3-EV5 */
		case 0xe: /* DM5/2-DH5 */
		case 0xf: /* DH5/3-DH5 */
			pld_item = proto_tree_add_item(btbb_tree, hf_btbb_payload, tvb, offset, -1, FALSE);
			break;
		default:
			break;
		}
	}

	/* Return the amount of data this dissector was able to dissect */
	return tvb_length(tvb);
}

/* register the protocol with Wireshark */
void
proto_register_btbb(void)
{

	/* list of fields */
	static hf_register_info hf[] = {
		{ &hf_btbb_pkthdr,
			{ "Packet Header", "btbb.pkthdr",
			FT_NONE, BASE_NONE, NULL, 0x0,
			"Bluetooth Baseband Packet Header", HFILL }
		},
		{ &hf_btbb_ltaddr,
			{ "LT_ADDR", "btbb.lt_addr",
			FT_UINT8, BASE_HEX, NULL, 0x07,
			"Logical Transport Address", HFILL }
		},
		{ &hf_btbb_type,
			{ "TYPE", "btbb.type",
			FT_UINT8, BASE_HEX, RVALS(packet_types), 0x78,
			"Packet Type", HFILL }
		},
		{ &hf_btbb_flags,
			{ "Flags", "btbb.flags",
			FT_UINT8, BASE_HEX, NULL, 0x0,
			"Packet Header Flags", HFILL }
		},
		{ &hf_btbb_flow,
			{ "FLOW", "btbb.flow",
			FT_BOOLEAN, 8, NULL, 0x01,
			"Flow control indication", HFILL }
		},
		{ &hf_btbb_arqn,
			{ "ARQN", "btbb.arqn",
			FT_BOOLEAN, 8, NULL, 0x02,
			"Acknowledgment indication", HFILL }
		},
		{ &hf_btbb_seqn,
			{ "SEQN", "btbb.seqn",
			FT_BOOLEAN, 8, NULL, 0x04,
			"Sequence number", HFILL }
		},
		{ &hf_btbb_hec,
			{ "HEC", "btbb.lt_addr",
			FT_UINT8, BASE_HEX, NULL, 0x0,
			"Header Error Check", HFILL }
		},
		{ &hf_btbb_payload,
			{ "Payload", "btbb.payload",
			FT_NONE, BASE_NONE, NULL, 0x0,
			"Payload", HFILL }
		},
	};

	/* protocol subtree arrays */
	static gint *ett[] = {
		&ett_btbb,
		&ett_btbb_pkthdr,
		&ett_btbb_flags,
		&ett_btbb_payload,
	};

	/* register the protocol name and description */
	proto_btbb = proto_register_protocol(
		"Bluetooth Baseband",	/* full name */
		"btbb",			/* short name */
		"btbb"			/* abbreviation (e.g. for filters) */
		);

	/* register the header fields and subtrees used */
	proto_register_field_array(proto_btbb, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));

}

void
proto_reg_handoff_btbb(void)
{
	static gboolean inited = FALSE;

	if (!inited) {
		dissector_handle_t btbb_handle;

		btbb_handle = new_create_dissector_handle(dissect_btbb, proto_btbb);
		/* hijacking this ethertype */
		dissector_add("ethertype", 0xFFF0, btbb_handle);

		inited = TRUE;
	}
}
