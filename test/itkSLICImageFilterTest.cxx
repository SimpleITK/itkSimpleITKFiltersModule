/*=========================================================================
 *
 *  Copyright Insight Software Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

#if defined( _MSC_VER ) && ! defined ( NDEBUG )
// Disable MSVC STL iterator debugging, as it's way too slow.
#define _HAS_ITERATOR_DEBUGGING 0
#endif
#include "itkSLICImageFilter.h"
#include "itkVectorImage.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkTimeProbe.h"

namespace
{

template<typename TImageType>
void itkSLICImageFilter(const std::string &inFileName,
                        const std::string &outFileName,
                        const unsigned int gridSize,
                        const float proximityWeight)
{

typedef TImageType                                             InputImageType;
typedef itk::Image<unsigned short, TImageType::ImageDimension> OutputImageType;

typedef itk::ImageFileReader<InputImageType> ReaderType;
typename ReaderType::Pointer reader = ReaderType::New();
reader->SetFileName(inFileName);

typedef itk::SLICImageFilter< InputImageType, OutputImageType > FilterType;
typename FilterType::Pointer filter = FilterType::New();
filter->SetInput(reader->GetOutput());
filter->SetSuperGridSize(gridSize);
filter->DebugOn();
filter->SetSpatialProximityWeight(proximityWeight);

reader->Update();

itk::TimeProbe clock;
clock.Start();
filter->Update();
clock.Stop();

std::cout << "Total: " << clock.GetTotal() << std::endl;


typedef itk::ImageFileWriter<OutputImageType> WriterType;
typename WriterType::Pointer writer = WriterType::New();
writer->SetFileName(outFileName);
writer->SetInput(filter->GetOutput());
writer->Update();

filter->Print(std::cout);

}
}

int itkSLICImageFilterTest(int argc, char *argv[])
{
  if (argc < 3)
    {
    std::cerr << "Expected inFileName outFileName [gridSize]\n";
    return EXIT_FAILURE;
    }


  const unsigned int gridSize = (argc > 3) ? atoi(argv[3] ) : 20;
  const float proximityWeight = (argc > 4) ? atof(argv[4] ) : 10.0;
  const char *inFileName = argv[1];
  const char *outFileName = argv[2];

  const unsigned int VDimension = 2;
  typedef itk::VectorImage<float, VDimension> InputImageType;

  typedef itk::ImageFileReader<InputImageType> ReaderType;
  ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileName(inFileName);
  reader->UpdateOutputInformation();

  const unsigned int Dimension = reader->GetImageIO()->GetNumberOfDimensions();
  const unsigned int Components = reader->GetImageIO()->GetNumberOfComponents();
  switch (Dimension)
    {
    case 1:
    case 2:
      if ( Components == 1 )
        {
          itkSLICImageFilter< itk::Image<float, 2> >(inFileName, outFileName, gridSize, proximityWeight);
        }
      else
        {
          itkSLICImageFilter< itk::VectorImage<float, 2> >(inFileName, outFileName, gridSize, proximityWeight);
        }
      break;
    case 3:
      if ( Components == 1 )
        {
        itkSLICImageFilter< itk::Image<float, 3> >(inFileName, outFileName, gridSize, proximityWeight);
        }
      else
        {
          itkSLICImageFilter< itk::VectorImage<float, 3> >(inFileName, outFileName, gridSize, proximityWeight);
        }
      break;
    default:
      std::cerr << "Unsupported Dimensions: " << Dimension << std::endl;
      return EXIT_FAILURE;
    }
  return 0;
}
