###########################################################################
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
###########################################################################

CC		?= cc
CFLAGS		?= -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror -D_POSIX_C_SOURCE=200809L -D_FILE_OFFSET_BITS=64
LDLIBS		?= -lm

PROGRAM		= wavmod
SOURCES		= file_io.c wav_file.c sample_store.c sample_reader.c
HEADERS		= file_io.h wav_file.h sample_store.h sample_reader.h

.PHONY: all clean

all: $(PROGRAM)

$(PROGRAM): main.c $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $(SOURCES) main.c $(LDLIBS) -o $@

clean:
	rm -f $(PROGRAM)
