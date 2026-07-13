#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <string>
#include <vector>

#include "graphics.h"
#include "log.h"
#include "logger.h"

namespace {
#ifdef GRAPHICS_USES_FONT
extern "C" {
extern const unsigned char kylin_embedded_font_start[];
extern const unsigned char kylin_embedded_font_end[];
}

void LogFontPathResult(const char *prefix, const char *path, int result)
{
	LOG_INFO("%s %s: 0x%08x", prefix, path, result);
}

std::vector<std::string> FontPathCandidates(const char *fontPath)
{
	std::vector<std::string> candidates;
	const char *installRoot = "/user/app/KYLN00002/";

	const char *app0Prefix = "/app0/";
	const size_t app0PrefixLen = strlen(app0Prefix);
	if (strncmp(fontPath, app0Prefix, app0PrefixLen) == 0) {
		candidates.push_back(std::string(installRoot) + (fontPath + app0PrefixLen));
	}

	candidates.push_back(fontPath);

	if (strncmp(fontPath, app0Prefix, app0PrefixLen) == 0) {
		candidates.push_back(fontPath + app0PrefixLen);
	}

	if (fontPath[0] == '/') {
		candidates.push_back(fontPath + 1);
	}

	return candidates;
}

bool ReadFileBytes(const std::string &path, std::vector<unsigned char> *bytes)
{
	int fd = sceKernelOpen(path.c_str(), O_RDONLY, 0);
	if (fd < 0) {
		LogFontPathResult("font open failed", path.c_str(), fd);
		return false;
	}

	off_t size = sceKernelLseek(fd, 0, SEEK_END);
	if (size <= 0 || sceKernelLseek(fd, 0, SEEK_SET) < 0) {
		LogFontPathResult("font seek failed", path.c_str(), static_cast<int>(size));
		sceKernelClose(fd);
		return false;
	}

	bytes->resize(static_cast<size_t>(size));
	size_t total = 0;
	while (total < bytes->size()) {
		size_t got = sceKernelRead(fd, bytes->data() + total, bytes->size() - total);
		if (got <= 0) {
			LogFontPathResult("font read failed", path.c_str(), static_cast<int>(got));
			sceKernelClose(fd);
			bytes->clear();
			return false;
		}
		total += static_cast<size_t>(got);
	}

	sceKernelClose(fd);
	return true;
}

bool InitFaceFromEmbeddedFont(FT_Library library, FT_Face *face, int fontSize)
{
	const unsigned char *start = kylin_embedded_font_start;
	const unsigned char *end = kylin_embedded_font_end;
	const size_t size = static_cast<size_t>(end - start);
	if (size == 0) {
		LOG_INFO("embedded font is empty");
		return false;
	}

	int rc = FT_New_Memory_Face(library, start, static_cast<FT_Long>(size), 0, face);
	if (rc != 0 || *face == NULL) {
		LOG_INFO("FT_New_Memory_Face embedded failed: 0x%08x", rc);
		LOG_INFO("FT_New_Memory_Face embedded face: %p", *face);
		return false;
	}

	rc = FT_Set_Pixel_Sizes(*face, 0, fontSize);
	if (rc != 0) {
		LOG_INFO("FT_Set_Pixel_Sizes embedded failed: 0x%08x", rc);
		return false;
	}

	LOG_INFO("embedded font bytes: %d (0x%08x)", static_cast<int>(size), static_cast<int>(size));
	return true;
}
#endif
}

Scene2D::Scene2D(int w, int h, int pixelDepth)
{
	this->width = w;
	this->height = h;
	this->depth = pixelDepth;

	this->frameBufferSize = this->width * this->height * this->depth;
}

bool Scene2D::Init(size_t memSize, int numFrameBuffers)
{
	int rc;

	this->video = sceVideoOutOpen(ORBIS_VIDEO_USER_MAIN, ORBIS_VIDEO_OUT_BUS_MAIN, 0, 0);
	this->videoMem = NULL;

	if (this->video < 0)
	{
		DEBUGLOG << "Failed to open a video out handle: " << std::string(strerror(errno));
		return false;
	}

#ifdef GRAPHICS_USES_FONT
	// Load freetype
	rc = sceSysmoduleLoadModule(ORBIS_SYSMODULE_FREETYPE_OL);

	if (rc < 0)
	{
		DEBUGLOG << "Failed to load freetype: " << std::string(strerror(errno));
		return false;
	}

	// Initialize freetype
	rc = FT_Init_FreeType(&this->ftLib);

	if (rc != 0)
	{
		DEBUGLOG << "Failed to initialize freetype: " << std::string(strerror(errno));
		return false;
	}
#endif

	if(!initFlipQueue())
	{
		DEBUGLOG << "Failed to initialize flip queue: " << std::string(strerror(errno));
		return false;
	}

	if(!allocateVideoMem(memSize, 0x200000))
	{
		DEBUGLOG << "Failed to allocate video memory: " << std::string(strerror(errno));
		return false;
	}

	if(!allocateFrameBuffers(numFrameBuffers))
	{
		DEBUGLOG << "Failed to allocate frame buffers: " << std::string(strerror(errno));
		return false;
	}

	sceVideoOutSetFlipRate(this->video, 0);
	return true;
}

bool Scene2D::initFlipQueue()
{
	int rc = sceKernelCreateEqueue(&flipQueue, "homebrew flip queue");

	if(rc < 0)
		return false;

	sceVideoOutAddFlipEvent(flipQueue, this->video, 0);
	return true;
}

bool Scene2D::allocateFrameBuffers(int num)
{
	// Allocate frame buffers array
	this->frameBuffers = new char*[num];

	// Set the display buffers
	for(int i = 0; i < num; i++)
		this->frameBuffers[i] = this->allocateDisplayMem(frameBufferSize);

	// Set SRGB pixel format
	sceVideoOutSetBufferAttribute(&this->attr, 0x80000000, 1, 0, this->width, this->height, this->width);

	// Register the buffers to the video handle
	return (sceVideoOutRegisterBuffers(this->video, 0, (void **)this->frameBuffers, num, &this->attr) == 0);
}

char *Scene2D::allocateDisplayMem(size_t size)
{
	// Essentially just bump allocation
	char *allocatedPtr = (char *)videoMemSP;
	videoMemSP += size;

	return allocatedPtr;
}

bool Scene2D::allocateVideoMem(size_t size, int alignment)
{
	int rc;

	// Align the allocation size
	this->directMemAllocationSize = (size + alignment - 1) / alignment * alignment;

	// Allocate memory for display buffer
	rc = sceKernelAllocateDirectMemory(0, sceKernelGetDirectMemorySize(), this->directMemAllocationSize, alignment, 3, &this->directMemOff);

	if(rc < 0)
	{
		this->directMemAllocationSize = 0;
		return false;
	}

	// Map the direct memory
	rc = sceKernelMapDirectMemory(&this->videoMem, this->directMemAllocationSize, 0x33, 0, this->directMemOff, alignment);

	if(rc < 0)
	{
		sceKernelReleaseDirectMemory(this->directMemOff, this->directMemAllocationSize);

		this->directMemOff = 0;
		this->directMemAllocationSize = 0;

		return false;
	}

	// Set the stack pointer to the beginning of the buffer
	this->videoMemSP = (uintptr_t)this->videoMem;
	return true;
}

void Scene2D::deallocateVideoMem()
{
	// Free the direct memory
	sceKernelReleaseDirectMemory(this->directMemOff, this->directMemAllocationSize);

	// Zero out meta data
	this->videoMem = 0;
	this->videoMemSP = 0;
	this->directMemOff = 0;
	this->directMemAllocationSize = 0;

	// Free the frame buffer array
	delete this->frameBuffers;
	this->frameBuffers = 0;
}

void Scene2D::SetActiveFrameBuffer(int index)
{
	this->activeFrameBufferIdx = index;
}

void Scene2D::SubmitFlip(int frameID)
{
	sceVideoOutSubmitFlip(this->video, this->activeFrameBufferIdx, ORBIS_VIDEO_OUT_FLIP_VSYNC, frameID);
}

void Scene2D::FrameWait(int frameID)
{
	OrbisKernelEvent evt;
	int count;

	// If the video handle is not initialized, bail out. This is mostly a failsafe, this should never happen.
	if(this->video == 0)
		return;

	for(;;)
	{
		OrbisVideoOutFlipStatus flipStatus;

		// Get the flip status and check the arg for the given frame ID
		sceVideoOutGetFlipStatus(video, &flipStatus);

		if(flipStatus.flipArg == frameID)
			break;

		// Wait on next flip event
		if(sceKernelWaitEqueue(this->flipQueue, &evt, 1, &count, 0) != 0)
			break;
	}
}

void Scene2D::FrameBufferSwap()
{
	// Swap the frame buffer for some perf
	this->activeFrameBufferIdx = (this->activeFrameBufferIdx + 1) % 2;
}

void Scene2D::FrameBufferClear()
{
	// Clear the screen with a white frame buffer
	Color blank = { 255, 255, 255 };
	FrameBufferFill(blank);
}

#ifdef GRAPHICS_USES_FONT
bool Scene2D::InitFont(FT_Face *face, const char *fontPath, int fontSize)
{
	int rc;

	const char *embeddedPrefix = "embedded:";
	if (strncmp(fontPath, embeddedPrefix, strlen(embeddedPrefix)) == 0) {
		return InitFaceFromEmbeddedFont(this->ftLib, face, fontSize);
	}

	for (const std::string &candidate : FontPathCandidates(fontPath)) {
		rc = FT_New_Face(this->ftLib, candidate.c_str(), 0, face);
		if(rc == 0 && *face != NULL)
		{
			rc = FT_Set_Pixel_Sizes(*face, 0, fontSize);
			if(rc != 0)
			{
				LOG_INFO("FT_Set_Pixel_Sizes failed: 0x%08x", rc);
				return false;
			}

			return true;
		}

		LogFontPathResult("FT_New_Face failed", candidate.c_str(), rc);
		LOG_INFO("FT_New_Face face: %p", *face);
	}

	for (const std::string &candidate : FontPathCandidates(fontPath)) {
		for (FontMemory &memory : this->fontMemory_) {
			if (memory.path == candidate) {
				rc = FT_New_Memory_Face(this->ftLib, memory.bytes.data(),
					static_cast<FT_Long>(memory.bytes.size()), 0, face);
				if (rc == 0 && *face != NULL) {
					rc = FT_Set_Pixel_Sizes(*face, 0, fontSize);
					if(rc != 0)
					{
						LOG_INFO("FT_Set_Pixel_Sizes failed: 0x%08x", rc);
						return false;
					}

					return true;
				}

				LogFontPathResult("FT_New_Memory_Face failed", candidate.c_str(), rc);
				return false;
			}
		}

		FontMemory memory;
		memory.path = candidate;
		if (!ReadFileBytes(candidate, &memory.bytes)) {
			continue;
		}

		this->fontMemory_.push_back(memory);
		FontMemory &stored = this->fontMemory_.back();
		rc = FT_New_Memory_Face(this->ftLib, stored.bytes.data(),
			static_cast<FT_Long>(stored.bytes.size()), 0, face);
		if (rc == 0 && *face != NULL) {
			rc = FT_Set_Pixel_Sizes(*face, 0, fontSize);
			if(rc != 0)
			{
				LOG_INFO("FT_Set_Pixel_Sizes failed: 0x%08x", rc);
				return false;
			}

			return true;
		}

		LogFontPathResult("FT_New_Memory_Face failed", candidate.c_str(), rc);
	}

	return false;
}
#endif

void Scene2D::FrameBufferFill(Color color)
{
	DrawRectangle(0, 0, this->width, this->height, color);
}

void Scene2D::DrawPixel(int x, int y, Color color)
{
	// Get pixel location based on pitch
	int pixel = (y * this->width) + x;

	// Encode to A8R8G8B8
	uint32_t encodedColor = 0x80000000 + (color.r << 16) + (color.g << 8) + color.b;

	// Draw to the frame buffer
	((uint32_t *)this->frameBuffers[this->activeFrameBufferIdx])[pixel] = encodedColor;
}

void Scene2D::DrawRectangle(int x, int y, int w, int h, Color color)
{
	int xPos, yPos;

	// Draw row-by-row, column-by-column
	for(yPos = y; yPos < y + h; yPos++)
	{
		for(xPos = x; xPos < x + w; xPos++)
		{
			DrawPixel(xPos, yPos, color);
		}
	}
}

#ifdef GRAPHICS_USES_FONT
int Scene2D::DrawCodepoint(uint32_t codepoint, FT_Face face, int startX, int baselineY, Color fgColor)
{
	if (face == NULL) {
		return 0;
	}

	FT_UInt glyph_index = FT_Get_Char_Index(face, codepoint);
	if (glyph_index == 0) {
		return 0;
	}

	int rc = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
	if (rc) {
		return 0;
	}

	FT_GlyphSlot slot = face->glyph;
	rc = FT_Render_Glyph(slot, ft_render_mode_normal);
	if (rc) {
		return slot->advance.x >> 6;
	}

	for (int yPos = 0; yPos < slot->bitmap.rows; yPos++)
	{
		for (int xPos = 0; xPos < slot->bitmap.width; xPos++)
		{
			unsigned char pixel = slot->bitmap.buffer[(yPos * slot->bitmap.pitch) + xPos];
			int x = startX + xPos + slot->bitmap_left;
			int y = baselineY + yPos - slot->bitmap_top;

			uint8_t r = (pixel * fgColor.r) / 255;
			uint8_t g = (pixel * fgColor.g) / 255;
			uint8_t b = (pixel * fgColor.b) / 255;

			Color finalColor = { r, g, b };

			if (x < 0 || y < 0 || x >= this->width || y >= this->height)
				continue;

			if(pixel != 0x00)
				this->DrawPixel(x, y, finalColor);
		}
	}

	return slot->advance.x >> 6;
}

void Scene2D::DrawText(char *txt, FT_Face face, int startX, int startY, Color bgColor, Color fgColor)
{
	int rc;
	int xOffset = 0;
	int yOffset = 0;

	// Get the glyph slot for bitmap and font metrics
	FT_GlyphSlot slot = face->glyph;

	// Iterate each character of the text to write to the screen
	for(int n = 0; n < strlen(txt); n++)
	{
		FT_UInt glyph_index;

        // Get the glyph for the ASCII code
        glyph_index = FT_Get_Char_Index(face, txt[n]);

        // Load and render in 8-bit color
        rc = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);

        if (rc)
            continue;

        rc = FT_Render_Glyph(slot, ft_render_mode_normal);

        if (rc)
            continue;

        // If we get a newline, increment the y offset, reset the x offset, and skip to the next character
        if (txt[n] == '\n')
        {
            xOffset = 0;
            yOffset += slot->bitmap.width * 2;

            continue;
        }

        // Parse and write the bitmap to the frame buffer
        for (int yPos = 0; yPos < slot->bitmap.rows; yPos++)
        {
            for (int xPos = 0; xPos < slot->bitmap.width; xPos++)
            {
                // Decode the 8-bit bitmap
                char pixel = slot->bitmap.buffer[(yPos * slot->bitmap.width) + xPos];

                // Get new pixel coordinates to account for the character position and baseline, as well as newlines
                int x = startX + xPos + xOffset + slot->bitmap_left;
                int y = startY + yPos + yOffset - slot->bitmap_top;

                // Linearly interpolate between the foreground and background for smoother rendering
				uint8_t r = (pixel * fgColor.r) / 255;
				uint8_t g = (pixel * fgColor.g) / 255;
				uint8_t b = (pixel * fgColor.b) / 255;

                // Create new color struct with lerp'd values
                Color finalColor = { r, g, b };

                // We need to do bounds checking before commiting the pixel write due to our transformations, or we
                // could write out-of-bounds of the frame buffer
                if (x < 0 || y < 0 || x >= this->width || y >= this->height)
                    continue;

                // If the pixel in the bitmap isn't blank, we'll draw it
                if(pixel != 0x00)
                    this->DrawPixel(x, y, finalColor);
            }
        }

        // Increment x offset for the next character
        xOffset += slot->advance.x >> 6;
    }
}
#endif
