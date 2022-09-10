/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * JPEG still image video source
 *
 * Copyright (C) 2018 Paul Elder
 *
 * Contact: Paul Elder <paul.elder@ideasonboard.com>
 */

#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <linux/videodev2.h>

#include "events.h"
#include "tools.h"
#include "v4l2.h"
#include "offline-source.h"
#include "video-buffers.h"
#include "../tof-sdk-interface.h"

struct offline_source {
	struct video_source src;

	unsigned int imgsize;
	void *imgdata;
};

#define to_offline_source(s) container_of(s, struct offline_source, src)

static void offline_source_destroy(struct video_source *s)
{
	struct offline_source *src = to_offline_source(s);

	if (src->imgdata)
		free(src->imgdata);
	free(src);
}

static int offline_source_set_format(struct video_source *s,
				  struct v4l2_pix_format *fmt)
{
	if (fmt->pixelformat != v4l2_fourcc('M', 'J', 'P', 'G')) {
		printf("invalid pixel format\n");
		return -EINVAL;
	}

	return 0;
}

static int offline_source_set_frame_rate(struct video_source *s, unsigned int fps)
{ return 0; }

static int offline_source_free_buffers(struct video_source *s)
{ return 0; }

static int offline_source_stream_on(struct video_source *s)
{ return 0; }

static int offline_source_stream_off(struct video_source *s)
{ return 0; }

static void offline_source_fill_buffer(struct video_source *s,
				   struct video_buffer *buf)
{
	struct offline_source *src = to_offline_source(s);

	memcpy(buf->mem, src->imgdata, src->imgsize);
	buf->bytesused = src->imgsize;
}

static const struct video_source_ops offline_source_ops = {
	.destroy = offline_source_destroy,
	.set_format = offline_source_set_format,
	.set_frame_rate = offline_source_set_frame_rate,
	.alloc_buffers = NULL,
	.export_buffers = NULL,
	.free_buffers = offline_source_free_buffers,
	.stream_on = offline_source_stream_on,
	.stream_off = offline_source_stream_off,
	.queue_buffer = NULL,
	.fill_buffer = offline_source_fill_buffer,
};

struct video_source *offline_video_source_create(const char *img_path)
{
	struct offline_source *src;
	int fd = -1;
	ssize_t ret = -1;

	printf("Creating offline video source '%s'\n", img_path);

	if (img_path == NULL)
		return NULL;

	src = malloc(sizeof *src);
	if (!src)
		return NULL;

	memset(src, 0, sizeof *src);
	src->src.ops = &offline_source_ops;

	fd = open(img_path, O_RDONLY);
	if (fd == -1) {
		printf("Unable to open offline image '%s'\n", img_path);
		goto err1;
	}

	src->imgsize = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	src->imgdata = malloc(src->imgsize);
	if (src->imgdata == NULL) {
		printf("Unable to allocate memory for offline image\n");
		src->imgsize = 0;
		goto err2;
	}

	ret = read(fd, src->imgdata, src->imgsize);
	if (ret < 0) {
		printf("Unable to read from offline image '%s'\n", img_path);
		src->imgsize = 0;
		goto err3;
	}
	
	close(fd);

	return &src->src;

err3:
	free(src->imgdata);
err2:
	close(fd);
err1:
	free(src);
	return NULL;
}

void offline_video_source_init(struct video_source *s, struct events *events)
{
	struct offline_source *src = to_offline_source(s);

	src->src.events = events;
}
