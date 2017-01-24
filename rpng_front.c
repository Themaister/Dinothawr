/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2013 - Hans-Kristian Arntzen
 * 
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include <file/nbio.h>
#include <formats/image.h>
#include <formats/rpng.h>
#include <stdlib.h>

#include "rpng_front.h"

bool rpng_load_image_argb(const char *path, uint32_t **data,
      unsigned *width, unsigned *height)
{
   int retval;
   size_t file_len;
   bool              ret = true;
   rpng_t          *rpng = NULL;
   void             *ptr = NULL;
   struct nbio_t* handle = (struct nbio_t*)nbio_open(path, NBIO_READ);

   if (!handle)
      goto end;

   nbio_begin_read(handle);

   while (!nbio_iterate(handle));

   ptr = nbio_get_ptr(handle, &file_len);

   if (!ptr)
   {
      ret = false;
      goto end;
   }

   rpng = rpng_alloc();

   if (!rpng)
   {
      ret = false;
      goto end;
   }

   if (!rpng_set_buf_ptr(rpng, (uint8_t*)ptr))
   {
      ret = false;
      goto end;
   }

   if (!rpng_start(rpng))
   {
      ret = false;
      goto end;
   }

   while (rpng_iterate_image(rpng));

   if (!rpng_is_valid(rpng))
   {
      ret = false;
      goto end;
   }
   
   do
   {
      retval = rpng_process_image(rpng,
            (void**)data, file_len, width, height);
   }while(retval == IMAGE_PROCESS_NEXT);

   if (retval == IMAGE_PROCESS_ERROR || retval == IMAGE_PROCESS_ERROR_END)
      ret = false;

end:
   if (handle)
      nbio_free(handle);
   if (rpng)
      rpng_free(rpng);
   rpng = NULL;
   if (!ret)
      free(*data);
   return ret;
}
