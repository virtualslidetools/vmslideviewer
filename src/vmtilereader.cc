#include "vmtilereader.h"
extern "C"
{
#include "openslide/openslide.h"
}
//#include "composite.h"

vmOpenSlideReader::vmOpenSlideReader(const char *filename)
{
  int retCode = 0;
  mLastStatus = 0;
  mOpenslide = openslide_open(filename);
  memset(mDefaultBkgd, 0, 4);

  if (!mOpenslide) 
  {
    const char *errorMsg = openslide_get_error(mOpenslide);
    if (errorMsg)
    {
      mErrorMsg = errorMsg;
    }
    else
    {
      mErrorMsg = "";
    }
    mLastStatus = VM_READ_ERROR;
  }
  if (mOpenslide && retCode==0)
  {
    mTileWidth = 256;
    mTileHeight = 256;
    const char * slideRgb = openslide_get_property_value(mOpenslide, OPENSLIDE_PROPERTY_NAME_BACKGROUND_COLOR);
    if (slideRgb)
    {
      memcpy(mDefaultBkgd, slideRgb, 3);
    }
  }
}


void vmOpenSlideReader::close()
{
  if (mOpenslide)
  {
    openslide_close(mOpenslide);
    mOpenslide = NULL;
  }
}


vmOpenSlideReader::~vmOpenSlideReader()
{
  if (mOpenslide)
  {
    close();
  }
}


double vmOpenSlideReader::getLevelDownsample(int32_t level)
{
  return openslide_get_level_downsample(mOpenslide, level); 
}


void vmOpenSlideReader::getLevelDimensions(int32_t level, int64_t *width, int64_t *height)
{
  openslide_get_level_dimensions(mOpenslide, level, width, height); 
}


int32_t vmOpenSlideReader::getTotalLevels()
{
  return openslide_get_level_count(mOpenslide); 
}


int32_t vmOpenSlideReader::getBestLevelForDownSample(double downSample)
{
  return openslide_get_best_level_for_downsample(mOpenslide, downSample);
}


void vmOpenSlideReader::read(uint32_t *data, int64_t x, int64_t y, int32_t level, int64_t width, int64_t height)
{
  openslide_read_region(mOpenslide, data, x, y, level, width, height);
}


QString vmOpenSlideReader::getPropertyValue(const char * name)
{
  QString value;
  const char *pValue = openslide_get_property_value(mOpenslide, name);
  if (pValue)
  {
    value = pValue;
  }
  return value;
}

