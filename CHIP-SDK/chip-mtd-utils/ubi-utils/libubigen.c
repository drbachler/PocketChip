/*
 * Copyright (c) International Business Machines Corp., 2006
 * Copyright (C) 2008 Nokia Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See
 * the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * Generating UBI images.
 *
 * Authors: Oliver Lohmann
 *          Artem Bityutskiy
 */

#define PROGRAM_NAME "libubigen"

#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include <mtd/ubi-media.h>
#include <mtd_swab.h>
#include <libubigen.h>
#include <crc32.h>
#include "common.h"

static void nand_pairing_dist3_get_info(const struct ubigen_info *ui, int page,
					struct nand_pairing_info *info)
{
        int lastpage = (ui->consolidated_peb_size / ui->min_io_size) - 1;
        int dist = 3;

        if (page == lastpage)
                dist = 2;

        if (!page || (page & 1)) {
                info->group = 0;
                info->pair = (page + 1) / 2;
        } else {
                info->group = 1;
                info->pair = (page + 1 - dist) / 2;
        }
}

static int nand_pairing_dist3_get_wunit(const struct ubigen_info *ui,
					const struct nand_pairing_info *info)
{
        int lastpair = ((ui->consolidated_peb_size / ui->min_io_size) - 1) / 2;
        int page = info->pair * 2;
        int dist = 3;

        if (!info->group && !info->pair)
                return 0;

        if (info->pair == lastpair && info->group)
                dist = 2;

        if (!info->group)
                page--;
        else if (info->pair)
                page += dist - 1;

        assert(page < ui->consolidated_peb_size / ui->min_io_size);

        return page;
}

static const struct nand_pairing_scheme dist3_pairing_scheme = {
        .ngroups = 2,
        .get_info = nand_pairing_dist3_get_info,
        .get_wunit = nand_pairing_dist3_get_wunit,
};

static void nand_pairing_slc_get_info(const struct ubigen_info *ui,
				      int wunit,
				      struct nand_pairing_info *info)
{
	assert(wunit < ui->consolidated_peb_size / ui->min_io_size);
	info->pair = wunit;
	info->group = 0;
}

static int nand_pairing_slc_get_wunit(const struct ubigen_info *ui,
				      const struct nand_pairing_info *info)
{
	if (info->group ||
	    info->pair >= ui->consolidated_peb_size / ui->min_io_size)
		return -EINVAL;

	return info->pair;
}

static const struct nand_pairing_scheme slc_pairing_scheme = {
	.ngroups = 1,
	.get_info = nand_pairing_slc_get_info,
	.get_wunit = nand_pairing_slc_get_wunit,
};

struct pairing_scheme_desc {
	const char *name;
	const struct nand_pairing_scheme *scheme;
};

static const struct pairing_scheme_desc pairing_schemes[] = {
	{ "dist3", &dist3_pairing_scheme },
	{ /* sentinel */ },
};

int mtd_pairing_info_to_wunit(const struct ubigen_info *ui,
                              const struct nand_pairing_info *info)
{
        return ui->pairing_scheme->get_wunit(ui, info);
}

void copy_soft_slc(const struct ubigen_info *ui, int offset, char *dst, char *src, size_t len)
{
	struct nand_pairing_info info;
	int chunklen, end = offset + len;

	info.pair = offset / ui->min_io_size;
	info.group = 0;

        while (offset < end) {
                int realoffs;

		if (ui->min_io_size < end - offset)
			chunklen = ui->min_io_size;
		else
			chunklen = end - offset;

                realoffs = mtd_pairing_info_to_wunit(ui, &info);
                realoffs *= ui->min_io_size;
		realoffs += offset % ui->min_io_size;
		assert(realoffs < ui->consolidated_peb_size);
		memcpy(dst + realoffs, src, chunklen);

                offset += chunklen;
                src += chunklen;
                info.pair++;
        }	
}

void ubigen_info_init(struct ubigen_info *ui, int peb_size, int min_io_size,
		      int subpage_size, int vid_hdr_offs, int ubi_ver,
		      uint32_t image_seq, const char *pairing_scheme)
{
	const struct nand_pairing_scheme *pscheme = &slc_pairing_scheme;

	if (!vid_hdr_offs) {
		vid_hdr_offs = UBI_EC_HDR_SIZE + subpage_size - 1;
		vid_hdr_offs /= subpage_size;
		vid_hdr_offs *= subpage_size;
	}

	if (pairing_scheme) {
		const struct pairing_scheme_desc *pairing_desc;

		for (pairing_desc = pairing_schemes; pairing_desc->name;
		     pairing_desc++) {
			if (!strcmp(pairing_scheme, pairing_desc->name))
				break;
		}

		if (!pairing_desc->name)
			errmsg("unknow pairing scheme (%s)", pairing_scheme);
		else
			pscheme = pairing_desc->scheme;
	}

	//TODO: prepare for new UBI versioning
	ui->clebs_per_peb = pscheme->ngroups;
	ui->pairing_scheme = pscheme;

	ui->consolidated_peb_size = peb_size;
	ui->peb_size = ui->consolidated_peb_size / ui->clebs_per_peb;
	ui->min_io_size = min_io_size;
	ui->vid_hdr_offs = vid_hdr_offs;
	ui->data_offs = vid_hdr_offs + UBI_VID_HDR_SIZE + min_io_size - 1;
	ui->data_offs /= min_io_size;
	ui->data_offs *= min_io_size;
	printf("data offset = %08x vid_hdr_offs = %08x\n", ui->data_offs, ui->vid_hdr_offs);
	ui->leb_size = ui->peb_size - ui->data_offs;
	ui->ubi_ver = ubi_ver;
	ui->image_seq = image_seq;

	ui->max_volumes = ui->leb_size / UBI_VTBL_RECORD_SIZE;
	if (ui->max_volumes > UBI_MAX_VOLUMES)
		ui->max_volumes = UBI_MAX_VOLUMES;
	ui->vtbl_size = ui->max_volumes * UBI_VTBL_RECORD_SIZE;
}

struct ubi_vtbl_record *ubigen_create_empty_vtbl(const struct ubigen_info *ui)
{
	struct ubi_vtbl_record *vtbl;
	int i;

	vtbl = calloc(1, ui->vtbl_size);
	if (!vtbl) {
		sys_errmsg("cannot allocate %d bytes of memory", ui->vtbl_size);
		return NULL;
	}

	for (i = 0; i < ui->max_volumes; i++) {
		uint32_t crc = mtd_crc32(UBI_CRC32_INIT, &vtbl[i],
				     UBI_VTBL_RECORD_SIZE_CRC);
		vtbl[i].crc = cpu_to_be32(crc);
	}

	return vtbl;
}

int ubigen_add_volume(const struct ubigen_info *ui,
		      const struct ubigen_vol_info *vi,
		      struct ubi_vtbl_record *vtbl)
{
	struct ubi_vtbl_record *vtbl_rec = &vtbl[vi->id];
	uint32_t tmp;

	if (vi->id >= ui->max_volumes) {
		errmsg("too high volume id %d, max. volumes is %d",
		       vi->id, ui->max_volumes);
		errno = EINVAL;
		return -1;
	}

	if (vi->alignment >= ui->leb_size) {
		errmsg("too large alignment %d, max is %d (LEB size)",
		       vi->alignment, ui->leb_size);
		errno = EINVAL;
		return -1;
	}

	memset(vtbl_rec, 0, sizeof(struct ubi_vtbl_record));
	tmp = (vi->bytes + ui->leb_size - 1) / ui->leb_size;
	vtbl_rec->reserved_pebs = cpu_to_be32(tmp);
	vtbl_rec->alignment = cpu_to_be32(vi->alignment);
	vtbl_rec->vol_type = vi->type;
	tmp = ui->leb_size % vi->alignment;
	vtbl_rec->data_pad = cpu_to_be32(tmp);
	vtbl_rec->flags = vi->flags;

	memcpy(vtbl_rec->name, vi->name, vi->name_len);
	vtbl_rec->name[vi->name_len] = '\0';
	vtbl_rec->name_len = cpu_to_be16(vi->name_len);

	tmp = mtd_crc32(UBI_CRC32_INIT, vtbl_rec, UBI_VTBL_RECORD_SIZE_CRC);
	vtbl_rec->crc =	 cpu_to_be32(tmp);
	return 0;
}

void ubigen_init_ec_hdr(const struct ubigen_info *ui,
		        struct ubi_ec_hdr *hdr, long long ec)
{
	uint32_t crc;

	memset(hdr, 0, sizeof(struct ubi_ec_hdr));

	hdr->magic = cpu_to_be32(UBI_EC_HDR_MAGIC);
	hdr->version = ui->ubi_ver;
	hdr->ec = cpu_to_be64(ec);
	hdr->vid_hdr_offset = cpu_to_be32(ui->vid_hdr_offs);
	hdr->data_offset = cpu_to_be32(ui->data_offs);
	hdr->image_seq = cpu_to_be32(ui->image_seq);

	crc = mtd_crc32(UBI_CRC32_INIT, hdr, UBI_EC_HDR_SIZE_CRC);
	hdr->hdr_crc = cpu_to_be32(crc);
}

void ubigen_init_vid_hdr(const struct ubigen_info *ui,
			 const struct ubigen_vol_info *vi,
			 struct ubi_vid_hdr *hdr, int lnum,
			 const void *data, int data_size)
{
	uint32_t crc;

	memset(hdr, 0, sizeof(struct ubi_vid_hdr));

	hdr->magic = cpu_to_be32(UBI_VID_HDR_MAGIC);
	hdr->version = ui->ubi_ver;
	hdr->vol_type = vi->type;
	hdr->vol_id = cpu_to_be32(vi->id);
	hdr->lnum = cpu_to_be32(lnum);
	hdr->data_pad = cpu_to_be32(vi->data_pad);
	hdr->compat = vi->compat;

	if (vi->type == UBI_VID_STATIC) {
		hdr->data_size = cpu_to_be32(data_size);
		hdr->used_ebs = cpu_to_be32(vi->used_ebs);
		crc = mtd_crc32(UBI_CRC32_INIT, data, data_size);
		hdr->data_crc = cpu_to_be32(crc);
	}

	crc = mtd_crc32(UBI_CRC32_INIT, hdr, UBI_VID_HDR_SIZE_CRC);
	hdr->hdr_crc = cpu_to_be32(crc);
}

static bool is_empty(char *buf, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		if ((uint8_t)buf[i] != (uint8_t)0xff) {
			return false;
		}
	}

	return true;
}

static bool can_consolidate(const struct ubigen_info *ui,
			    const struct ubigen_vol_info *vi,
			    char *buf)
{
	char *last_page = buf + vi->usable_leb_size - ui->min_io_size;

	if (ui->pairing_scheme->ngroups < 2)
		return false;

	return !is_empty(last_page, ui->min_io_size);
}

int ubigen_write_volume(const struct ubigen_info *ui,
			const struct ubigen_vol_info *vi, long long ec,
			long long bytes, int in, int out)
{
	int len = vi->usable_leb_size, rd, lnum = 0, conspos = 0;
	char *inbuf, *consbuf, *outbuf;
	struct ubi_vid_hdr *vid_hdrs;

	if (vi->id >= ui->max_volumes) {
		errmsg("too high volume id %d, max. volumes is %d",
		       vi->id, ui->max_volumes);
		errno = EINVAL;
		return -1;
	}

	if (vi->alignment >= ui->leb_size) {
		errmsg("too large alignment %d, max is %d (LEB size)",
		       vi->alignment, ui->leb_size);
		errno = EINVAL;
		return -1;
	}

	inbuf = malloc(len);
	if (!inbuf)
		return sys_errmsg("cannot allocate %d bytes of memory",
				  len * ui->clebs_per_peb);

	outbuf = malloc(ui->consolidated_peb_size * 2);
	if (!outbuf) {
		sys_errmsg("cannot allocate %d bytes of memory", ui->consolidated_peb_size);
		goto out_free;
	}

	consbuf = outbuf + ui->consolidated_peb_size;
	vid_hdrs = (struct ubi_vid_hdr *)(consbuf + ui->vid_hdr_offs);
	memset(outbuf, 0xFF, ui->consolidated_peb_size);
	memset(consbuf, 0xFF, ui->consolidated_peb_size);
	ubigen_init_ec_hdr(ui, (struct ubi_ec_hdr *)outbuf, ec);
	ubigen_init_ec_hdr(ui, (struct ubi_ec_hdr *)consbuf, ec);

	while (bytes) {
		int readlen = len;
		bool full;

		if (bytes < readlen) {
			memset(inbuf + bytes, 0xFF, readlen - bytes);
			readlen = bytes;
		}
		bytes -= readlen;


		rd = read(in, inbuf, readlen);
		if (rd != readlen) {
			sys_errmsg("cannot read %d bytes from the input file", readlen);
			goto out_free1;
		}

		full = can_consolidate(ui, vi, inbuf);
		if (full) {
			if (!conspos)
				memset(consbuf + ui->vid_hdr_offs, 0,
				       ui->consolidated_peb_size -
				       ui->vid_hdr_offs);
			ubigen_init_vid_hdr(ui, vi, &vid_hdrs[conspos],
					    lnum, inbuf, len);
			memcpy(consbuf + ui->data_offs + (len * conspos),
			       inbuf, len);

			if (++conspos == ui->clebs_per_peb) {
				if (write(out, consbuf, ui->consolidated_peb_size) !=
				    ui->consolidated_peb_size) {
					sys_errmsg("cannot write %d bytes to the output file",
						   ui->consolidated_peb_size);
					goto out_free1;
				}
				conspos = 0;
			}
			lnum++;
		} else {
			struct ubi_vid_hdr vid_hdr;

			assert(len <= vi->usable_leb_size);

			memset(outbuf + ui->vid_hdr_offs, 0, ui->min_io_size);
			memset(outbuf + ui->data_offs, 0xff, ui->leb_size);

			ubigen_init_vid_hdr(ui, vi, &vid_hdr, lnum, inbuf, len);
			copy_soft_slc(ui, ui->vid_hdr_offs, outbuf, (void *)&vid_hdr, sizeof(vid_hdr));
			copy_soft_slc(ui, ui->data_offs, outbuf, inbuf, len);
			lnum++;

			if (write(out, outbuf, ui->consolidated_peb_size) !=
			    ui->consolidated_peb_size) {
				sys_errmsg("cannot write %d bytes to the output file",
					   ui->consolidated_peb_size);
				goto out_free1;
			}
		}
	}

	if (conspos) {
		if (conspos > 1) {
			memcpy(outbuf, consbuf, ui->consolidated_peb_size);
		} else {
			memcpy(outbuf + ui->vid_hdr_offs,
			       vid_hdrs, sizeof(*vid_hdrs));
			copy_soft_slc(ui, ui->data_offs, outbuf,
				      consbuf + ui->data_offs, len);
		}

		if (write(out, outbuf, ui->consolidated_peb_size) !=
		    ui->consolidated_peb_size) {
			sys_errmsg("cannot write %d bytes to the output file",
				   ui->consolidated_peb_size);
			goto out_free1;
		}
	}

	free(outbuf);
	free(inbuf);
	return 0;

out_free1:
	free(outbuf);
out_free:
	free(inbuf);
	return -1;
}

static int __write_layout_vol(const struct ubigen_info *ui, const struct ubigen_vol_info *vi,
			      int peb, int lnum, long long ec, struct ubi_vtbl_record *vtbl, int fd)
{
	int ret;
	char *outbuf;
	struct ubi_vid_hdr *vid_hdr;
	off_t seek;

	outbuf = malloc(ui->peb_size);
	if (!outbuf)
		return sys_errmsg("failed to allocate %d bytes",
				  ui->peb_size);

	memset(outbuf, 0xFF, ui->data_offs);
	vid_hdr = (struct ubi_vid_hdr *)(&outbuf[ui->vid_hdr_offs]);
	printf("data_offset %08x\n", ui->data_offs);
	memcpy(outbuf + ui->data_offs, vtbl, ui->vtbl_size);
	memset(outbuf + ui->data_offs + ui->vtbl_size, 0xFF,
	       ui->peb_size - ui->data_offs - ui->vtbl_size);
	memset(outbuf + ui->vid_hdr_offs, 0, ui->data_offs - ui->vid_hdr_offs);

	seek = (off_t) peb * ui->peb_size;
	if (lseek(fd, seek, SEEK_SET) != seek) {
		sys_errmsg("cannot seek output file");
		goto out_free;
	}

	ubigen_init_ec_hdr(ui, (struct ubi_ec_hdr *)outbuf, ec);
	ubigen_init_vid_hdr(ui, vi, vid_hdr, lnum, NULL, 0);
	ret = write(fd, outbuf, ui->peb_size);
	if (ret != ui->peb_size) {
		sys_errmsg("cannot write %d bytes", ui->peb_size);
		goto out_free;
	}

	free(outbuf);
	return 0;

out_free:
	free(outbuf);
	return -1;
}

static int __write_layout_vol2(const struct ubigen_info *ui, const struct ubigen_vol_info *vi,
			       int peb, long long ec, struct ubi_vtbl_record *vtbl, int fd)
{
	int ret, i;
	char *outbuf;
	struct ubi_vid_hdr *vid_hdrs;
	off_t seek;

	outbuf = malloc(ui->consolidated_peb_size);
	if (!outbuf)
		return sys_errmsg("failed to allocate %d bytes",
				  ui->consolidated_peb_size);

	memset(outbuf, 0x00, ui->consolidated_peb_size);
	vid_hdrs = (struct ubi_vid_hdr *)(&outbuf[ui->vid_hdr_offs]);
	memset(outbuf + ui->vid_hdr_offs, 0, ui->min_io_size);

	seek = (off_t) peb * ui->consolidated_peb_size;
	if (lseek(fd, seek, SEEK_SET) != seek) {
		sys_errmsg("cannot seek output file");
		goto out_free;
	}

	ubigen_init_ec_hdr(ui, (struct ubi_ec_hdr *)outbuf, ec);

	for (i = 0; i < ui->clebs_per_peb; i++) {
		ubigen_init_vid_hdr(ui, vi, &vid_hdrs[i], i, NULL, 0);
		memcpy(outbuf + ui->data_offs + (vi->usable_leb_size * i), vtbl, ui->vtbl_size);
	}

	ret = write(fd, outbuf, ui->consolidated_peb_size);
	if (ret != ui->consolidated_peb_size) {
		sys_errmsg("cannot write %d bytes", ui->consolidated_peb_size);
		goto out_free;
	}

	free(outbuf);
	return 0;

out_free:
	free(outbuf);
	return -1;
}

int ubigen_write_layout_vol(const struct ubigen_info *ui, int peb1, int peb2,
			    long long ec1, long long ec2,
			    struct ubi_vtbl_record *vtbl, int fd)
{
	int ret;
	struct ubigen_vol_info vi;

	vi.bytes = ui->leb_size * UBI_LAYOUT_VOLUME_EBS;
	vi.id = UBI_LAYOUT_VOLUME_ID;
	vi.alignment = UBI_LAYOUT_VOLUME_ALIGN;
	vi.data_pad = ui->leb_size % UBI_LAYOUT_VOLUME_ALIGN;
	vi.usable_leb_size = ui->leb_size - vi.data_pad;
	vi.data_pad = ui->leb_size - vi.usable_leb_size;
	vi.type = UBI_LAYOUT_VOLUME_TYPE;
	vi.name = UBI_LAYOUT_VOLUME_NAME;
	vi.name_len = strlen(UBI_LAYOUT_VOLUME_NAME);
	vi.compat = UBI_LAYOUT_VOLUME_COMPAT;

	if (ui->clebs_per_peb > 1)
		return __write_layout_vol2(ui, &vi, peb1, ec1, vtbl, fd);

	ret = __write_layout_vol(ui, &vi, peb1, 0, ec1, vtbl, fd);
	if (ret)
		goto out;

	ret = __write_layout_vol(ui, &vi, peb2, 1, ec2, vtbl, fd);
	if (ret)
		goto out;

out:
	return ret;
}
