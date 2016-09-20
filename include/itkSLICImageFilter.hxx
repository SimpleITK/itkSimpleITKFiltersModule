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
#ifndef itkSLICImageFilter_hxx
#define itkSLICImageFilter_hxx

#include "itkSLICImageFilter.h"


#include "itkShrinkImageFilter.h"
#include "itkConstNeighborhoodIterator.h"
#include "itkImageRegionIterator.h"


namespace itk
{

template<typename TInputImage, typename TOutputImage, typename TDistancePixel>
SLICImageFilter<TInputImage, TOutputImage, TDistancePixel>
::SLICImageFilter()
  : m_MaximumNumberOfIterations( (ImageDimension > 2) ? 5 : 10),
    m_SpatialProximityWeight( 10.0 ),
    m_Barrier(Barrier::New())
{
  m_SuperGridSize.Fill(50);
}

template<typename TInputImage, typename TOutputImage, typename TDistancePixel>
SLICImageFilter<TInputImage, TOutputImage, TDistancePixel>
::~SLICImageFilter()
{
}


template<typename TInputImage, typename TOutputImage, typename TDistancePixel>
void
SLICImageFilter<TInputImage, TOutputImage, TDistancePixel>
::SetSuperGridSize(unsigned int factor)
{
  unsigned int i;
  for (i = 0; i < ImageDimension; ++i)
    {
    if (factor != m_SuperGridSize[i])
      {
      break;
      }
    }
  if ( i < ImageDimension )
    {
    this->Modified();
    m_SuperGridSize.Fill(factor);
    }
}

template<typename TInputImage, typename TOutputImage, typename TDistancePixel>
void
SLICImageFilter<TInputImage, TOutputImage, TDistancePixel>
::SetSuperGridSize(unsigned int i, unsigned int factor)
{
  if (m_SuperGridSize[i] == factor)
    {
    return;
    }

  this->Modified();
  m_SuperGridSize[i] = factor;
}

template<typename TInputImage, typename TOutputImage, typename TDistancePixel>
void
SLICImageFilter<TInputImage, TOutputImage, TDistancePixel>
::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);
  os << indent << "SuperGridSize: " << m_SuperGridSize << std::endl;
  os << indent << "MaximumNumberOfIterations: " << m_MaximumNumberOfIterations << std::endl;
  os << indent << "SpatialProximityWeight: " << m_SpatialProximityWeight << std::endl;
}

template<typename TInputImage, typename TOutputImage, typename TDistancePixel>
void
SLICImageFilter<TInputImage, TOutputImage, TDistancePixel>
::EnlargeOutputRequestedRegion(DataObject *output)
{
  Superclass::EnlargeOutputRequestedRegion(output);
  output->SetRequestedRegionToLargestPossibleRegion();
}


template<typename TInputImage, typename TOutputImage, typename TDistancePixel>
void
SLICImageFilter<TInputImage, TOutputImage, TDistancePixel>
::BeforeThreadedGenerateData()
{
  itkDebugMacro("Starting BeforeThreadedGenerateData");


  ThreadIdType numberOfThreads = this->GetNumberOfThreads();

  if ( itk::MultiThreader::GetGlobalMaximumNumberOfThreads() != 0 )
    {
    numberOfThreads = vnl_math_min(
      this->GetNumberOfThreads(), itk::MultiThreader::GetGlobalMaximumNumberOfThreads() );
    }

  // number of threads can be constrained by the region size, so call the
  // SplitRequestedRegion to get the real number of threads which will be used
  typename TOutputImage::RegionType splitRegion;  // dummy region - just to call
  // the following method

  numberOfThreads = this->SplitRequestedRegion(0, numberOfThreads, splitRegion);

  m_Barrier->Initialize(numberOfThreads);

  typename InputImageType::Pointer inputImage = InputImageType::New();
  inputImage->Graft( const_cast<  InputImageType * >( this->GetInput() ));


  itkDebugMacro("Shrinking Starting");
  typename InputImageType::Pointer shrunkImage;
  {
  // todo disconnect input from pipeline
  typedef itk::ShrinkImageFilter<InputImageType, InputImageType> ShrinkImageFilterType;
  typename ShrinkImageFilterType::Pointer shrinker = ShrinkImageFilterType::New();
  shrinker->SetInput(inputImage);
  shrinker->SetShrinkFactors(m_SuperGridSize);
  shrinker->UpdateLargestPossibleRegion();

  shrunkImage = shrinker->GetOutput();
  }
  itkDebugMacro("Shinking Completed")

  const typename InputImageType::RegionType region = inputImage->GetBufferedRegion();
  const unsigned int numberOfComponents = inputImage->GetNumberOfComponentsPerPixel();
  const unsigned int numberOfClusterComponents = numberOfComponents+ImageDimension;
  const size_t numberOfClusters = shrunkImage->GetBufferedRegion().GetNumberOfPixels();


  // allocate array of scalars
  m_Clusters.resize(numberOfClusters*numberOfClusterComponents);
  m_OldClusters.resize(numberOfClusters*numberOfClusterComponents);


  typedef ImageScanlineConstIterator< InputImageType > InputConstIteratorType;

  InputConstIteratorType it(shrunkImage, shrunkImage->GetLargestPossibleRegion());

  // Initialize cluster centers
  size_t cnt = 0;
  while(!it.IsAtEnd())
    {
    const size_t         ln =  shrunkImage->GetLargestPossibleRegion().GetSize(0);
    for (unsigned x = 0; x < ln; ++x)
      {
      // construct vector as reference to the scalar array
      ClusterType cluster( numberOfClusterComponents, &m_Clusters[cnt*numberOfClusterComponents] );

      const InputPixelType &v = it.Get();
      for(unsigned int i = 0; i < numberOfComponents; ++i)
        {
        cluster[i] = v[i];
        }
      const IndexType & idx = it.GetIndex();
      typename InputImageType::PointType pt;
      shrunkImage->TransformIndexToPhysicalPoint(idx, pt);
      for(unsigned int i = 0; i < ImageDimension; ++i)
        {
        cluster[numberOfComponents+i] = pt[i];
        }
      ++it;
      ++cnt;
      }
    it.NextLine();
    }
  itkDebugMacro("Initial Clustering Completed");

  shrunkImage = ITK_NULLPTR;

  // TODO: Move cluster center to lowest gradient position in a 3x
  // neighborhood


  m_DistanceImage = DistanceImageType::New();
  m_DistanceImage->CopyInformation(inputImage);
  m_DistanceImage->SetBufferedRegion( region );
  m_DistanceImage->Allocate();

  for (unsigned int i = 0; i < ImageDimension; ++i)
    {
    m_DistanceScales[i] = m_SpatialProximityWeight/m_SuperGridSize[i];
    }


  m_UpdateClusterPerThread.resize(numberOfThreads);

  this->Superclass::BeforeThreadedGenerateData();
}


template<typename TInputImage, typename TOutputImage, typename TDistancePixel>
void
SLICImageFilter<TInputImage, TOutputImage, TDistancePixel>
::ThreadedUpdateDistanceAndLabel(const OutputImageRegionType & outputRegionForThread, ThreadIdType  itkNotUsed(threadId))
{
  // This method modifies the OutputImage and the DistanceImage only
  // in the outputRegionForThread. It searches for any cluster, whose
  // search radius is within the output region for the thread. Then it
  // updates DistnaceImage with the minimum distance and the
  // corresponding label id in the output image.
  //
  typedef ImageScanlineConstIterator< InputImageType > InputConstIteratorType;
  typedef ImageScanlineIterator< DistanceImageType >   DistanceIteratorType;

  const InputImageType *inputImage = this->GetInput();
  OutputImageType *outputImage = this->GetOutput();
  const unsigned int numberOfComponents = inputImage->GetNumberOfComponentsPerPixel();
  const unsigned int numberOfClusterComponents = numberOfComponents+ImageDimension;

  typename InputImageType::SizeType searchRadius;
  for (unsigned int i = 0; i < ImageDimension; ++i)
    {
    searchRadius[i] = m_SuperGridSize[i];
    }

  for (size_t i = 0; i*numberOfClusterComponents < m_Clusters.size(); ++i)
    {
    ClusterType cluster(numberOfClusterComponents, &m_Clusters[i*numberOfClusterComponents]);
    typename InputImageType::RegionType localRegion;
    typename InputImageType::PointType pt;
    IndexType idx;

    for (unsigned int d = 0; d < ImageDimension; ++d)
      {
      pt[d] = cluster[numberOfComponents+d];
      }
    //std::cout << "Cluster " << i << "@" << pt <<": " << cluster << std::endl;
    inputImage->TransformPhysicalPointToIndex(pt, idx);

    localRegion.SetIndex(idx);
    localRegion.GetModifiableSize().Fill(1u);
    localRegion.PadByRadius(searchRadius);

    // Check cluster is in the output region for this thread.
    if (!localRegion.Crop(outputRegionForThread))
      {
      continue;
      }


    const size_t         ln =  localRegion.GetSize(0);

    InputConstIteratorType inputIter(inputImage, localRegion);
    DistanceIteratorType   distanceIter(m_DistanceImage, localRegion);


    while ( !inputIter.IsAtEnd() )
      {
      for( size_t x = 0; x < ln; ++x )
        {
        const IndexType &currentIdx = inputIter.GetIndex();

        inputImage->TransformIndexToPhysicalPoint(currentIdx, pt);
        const double distance = this->Distance(cluster,
                                               inputIter.Get(),
                                               pt);
        if (distance < distanceIter.Get() )
          {
          distanceIter.Set(distance);
          outputImage->SetPixel(currentIdx, i);
          }

        ++distanceIter;
        ++inputIter;
        }
      inputIter.NextLine();
      distanceIter.NextLine();
      }

    // for neighborhood iterator size S
    }

}


template<typename TInputImage, typename TOutputImage, typename TDistancePixel>
void
SLICImageFilter<TInputImage, TOutputImage, TDistancePixel>
::ThreadedUpdateClusters(const OutputImageRegionType & updateRegionForThread, ThreadIdType threadId)
{
  const InputImageType *inputImage = this->GetInput();
  OutputImageType *outputImage = this->GetOutput();

  const unsigned int numberOfComponents = inputImage->GetNumberOfComponentsPerPixel();
  const unsigned int numberOfClusterComponents = numberOfComponents+ImageDimension;

  typedef ImageScanlineConstIterator< InputImageType > InputConstIteratorType;
  typedef ImageScanlineIterator< OutputImageType >     OutputIteratorType;

  UpdateClusterMap &clusterMap = m_UpdateClusterPerThread[threadId];
  clusterMap.clear();

  itkDebugMacro("Estimating Centers");
  // calculate new centers
  OutputIteratorType itOut = OutputIteratorType(outputImage, updateRegionForThread);
  InputConstIteratorType itIn = InputConstIteratorType(inputImage, updateRegionForThread);
  while(!itOut.IsAtEnd() )
    {
    const size_t         ln =  updateRegionForThread.GetSize(0);
    for (unsigned x = 0; x < ln; ++x)
      {
      const IndexType &idx = itOut.GetIndex();
      const InputPixelType &v = itIn.Get();
      const typename OutputImageType::PixelType l = itOut.Get();

      std::pair<typename UpdateClusterMap::iterator, bool> r =  clusterMap.insert(std::make_pair(l,UpdateCluster()));
      vnl_vector<ClusterComponentType> &cluster = r.first->second.cluster;
      if (r.second)
        {
        cluster.set_size(numberOfClusterComponents);
        cluster.fill(0.0);
        r.first->second.count = 0;
        }
      ++r.first->second.count;

      for(unsigned int i = 0; i < numberOfComponents; ++i)
        {
        cluster[i] += v[i];
        }

      typename InputImageType::PointType pt;
      inputImage->TransformIndexToPhysicalPoint(idx, pt);
      for(unsigned int i = 0; i < ImageDimension; ++i)
        {
        cluster[numberOfComponents+i] += pt[i];
        }

      ++itIn;
      ++itOut;
      }
    itIn.NextLine();
    itOut.NextLine();
    }
}


template<typename TInputImage, typename TOutputImage, typename TDistancePixel>
void
SLICImageFilter<TInputImage, TOutputImage, TDistancePixel>
::ThreadedPerturbClusters(const OutputImageRegionType & outputRegionForThread, ThreadIdType itkNotUsed(threadId) )
{
  // Update the m_Clusters array by moving cluster center to the
  // lowest gradient position in a 1-radius neighborhood.

  const InputImageType *inputImage = this->GetInput();

  const unsigned int ImageDimension = TInputImage::ImageDimension;


  const unsigned int numberOfComponents = inputImage->GetNumberOfComponentsPerPixel();
  const unsigned int numberOfClusterComponents = numberOfComponents+ImageDimension;

  itk::Size<ImageDimension> radius;
  radius.Fill( 1 );
  unsigned long center;
  unsigned long stride[ImageDimension];


  typename InputImageType::SizeType searchRadius;
  searchRadius.Fill(1);


  typedef ConstNeighborhoodIterator< TInputImage > NeighborhoodType;

  // get center and dimension strides for iterator neighborhoods
  NeighborhoodType it( radius, inputImage, outputRegionForThread );
  center = it.Size()/2;
  for ( unsigned int i = 0; i < ImageDimension; ++i )
    {
    stride[i] = it.GetStride(i);
    }


  const typename InputImageType::SpacingType spacing = inputImage->GetSpacing();

  typedef typename NumericTraits<InputPixelType>::RealType GradientType;
  GradientType G;


  for (size_t clusterIndex = 0; clusterIndex*numberOfClusterComponents < m_Clusters.size(); ++clusterIndex)
    {
    // cluster is a reference to array
    ClusterType cluster(numberOfClusterComponents, &m_Clusters[clusterIndex*numberOfClusterComponents]);
    typename InputImageType::RegionType localRegion;
    typename InputImageType::PointType pt;
    IndexType idx;

    for (unsigned int d = 0; d < ImageDimension; ++d)
      {
      pt[d] = cluster[numberOfComponents+d];
      }
    inputImage->TransformPhysicalPointToIndex(pt, idx);

    if (!outputRegionForThread.IsInside(idx))
      {
      continue;
      }

    localRegion.SetIndex(idx);
    localRegion.GetModifiableSize().Fill(1u);
    localRegion.PadByRadius(searchRadius);


    it.SetRegion( localRegion );

    double minG = NumericTraits<double>::max();

    IndexType minIdx = idx;

    while ( !it.IsAtEnd() )
      {

      G = it.GetPixel(center + stride[0]);
      G -= it.GetPixel(center - stride[0]);
      G /= 2.0*spacing[0];

      for ( unsigned int i = 1; i < ImageDimension; i++ )
        {
        GradientType temp = it.GetPixel(center + stride[i]);
        temp -= it.GetPixel(center - stride[i]);
        temp /= 2.0*spacing[i];
        // todo need to square?
        G += temp;
        }

      const double gNorm = G.GetSquaredNorm();
      if ( gNorm < minG)
        {
        minG = gNorm;
        minIdx = it.GetIndex();
        }
      ++it;
      }

    const InputPixelType &v = inputImage->GetPixel(minIdx);
    for(unsigned int i = 0; i < numberOfComponents; ++i)
      {
      cluster[i] = v[i];
      }

    inputImage->TransformIndexToPhysicalPoint(minIdx, pt);
    for(unsigned int i = 0; i < ImageDimension; ++i)
      {
      cluster[numberOfComponents+i] = pt[i];
      }


    }

}


template<typename TInputImage, typename TOutputImage, typename TDistancePixel>
void
SLICImageFilter<TInputImage, TOutputImage, TDistancePixel>
::ThreadedGenerateData(const OutputImageRegionType & outputRegionForThread, ThreadIdType threadId)
{
  const InputImageType *inputImage = this->GetInput();

  const typename InputImageType::RegionType region = inputImage->GetBufferedRegion();
  const unsigned int numberOfComponents = inputImage->GetNumberOfComponentsPerPixel();
  const unsigned int numberOfClusterComponents = numberOfComponents+ImageDimension;

  itkDebugMacro("Perturb cluster centers");
  ThreadedPerturbClusters(outputRegionForThread,threadId);
  m_Barrier->Wait();

  itkDebugMacro("Entering Main Loop");
  for(unsigned int loopCnt = 0;  loopCnt<m_MaximumNumberOfIterations; ++loopCnt)
    {
    itkDebugMacro("Iteration :" << loopCnt);

    if (threadId == 0)
      {
      m_DistanceImage->FillBuffer(NumericTraits<typename DistanceImageType::PixelType>::max());
      }
    m_Barrier->Wait();

    ThreadedUpdateDistanceAndLabel(outputRegionForThread,threadId);

    m_Barrier->Wait();


    ThreadedUpdateClusters(outputRegionForThread, threadId);

    m_Barrier->Wait();

    if (threadId==0)
      {
      // prepare to update clusters
      swap(m_Clusters, m_OldClusters);
      std::fill(m_Clusters.begin(), m_Clusters.end(), 0.0);
      std::vector<size_t> clusterCount(m_Clusters.size()/numberOfClusterComponents, 0);

      // reduce the produce cluster maps per-thread into m_Cluster array
      for(unsigned int i = 0; i < m_UpdateClusterPerThread.size(); ++i)
        {
        UpdateClusterMap &clusterMap = m_UpdateClusterPerThread[i];
        for(typename UpdateClusterMap::const_iterator clusterIter = clusterMap.begin(); clusterIter != clusterMap.end(); ++clusterIter)
          {
          const size_t clusterIdx = clusterIter->first;
          clusterCount[clusterIdx] += clusterIter->second.count;

          ClusterType cluster(numberOfClusterComponents, &m_Clusters[clusterIdx*numberOfClusterComponents]);
          cluster += clusterIter->second.cluster;
          }
        }

      // average, l1
      double l1Residual = 0.0;
      for (size_t i = 0; i*numberOfClusterComponents < m_Clusters.size(); ++i)
        {

        ClusterType cluster(numberOfClusterComponents,&m_Clusters[i*numberOfClusterComponents]);
        cluster /= clusterCount[i];

        ClusterType oldCluster(numberOfClusterComponents, &m_OldClusters[i*numberOfClusterComponents]);
        l1Residual += Distance(cluster,oldCluster);

        }

      std::cout << "L1 residual: " << std::sqrt(l1Residual) << std::endl;
      }
    // while error <= threshold
    }


}

template<typename TInputImage, typename TOutputImage, typename TDistancePixel>
void
SLICImageFilter<TInputImage, TOutputImage, TDistancePixel>
::AfterThreadedGenerateData()
{
  // This method should clean up memory allocated during execution

  itkDebugMacro("Starting AfterThreadedGenerateData");

  m_DistanceImage = ITK_NULLPTR;

  // cleanup
  std::vector<ClusterComponentType>().swap(m_Clusters);
  std::vector<ClusterComponentType>().swap(m_OldClusters);
  for(unsigned int i = 0; i < m_UpdateClusterPerThread.size(); ++i)
    {
    UpdateClusterMap().swap(m_UpdateClusterPerThread[i]);
    }
}


template<typename TInputImage, typename TOutputImage, typename TDistancePixel>
typename SLICImageFilter<TInputImage, TOutputImage, TDistancePixel>::DistanceType
SLICImageFilter<TInputImage, TOutputImage, TDistancePixel>
::Distance(const ClusterType &cluster1, const ClusterType &cluster2)
{
  const unsigned int s = cluster1.size();
  DistanceType d1 = 0.0;
  DistanceType d2 = 0.0;
  unsigned int i = 0;
  for (; i<s-ImageDimension; ++i)
    {
    const DistanceType d = (cluster1[i] - cluster2[i]);
    d1 += d*d;
    }
  //d1 = std::sqrt(d1);

  for (unsigned int j = 0; j < ImageDimension; ++j)
    {
    const DistanceType d = (cluster1[i] - cluster2[i]) * m_DistanceScales[j];
    d2 += d*d;
    ++i;
    }
  //d2 = std::sqrt(d2);
  return d1+d2;
}

template<typename TInputImage, typename TOutputImage, typename TDistancePixel>
typename SLICImageFilter<TInputImage, TOutputImage, TDistancePixel>::DistanceType
SLICImageFilter<TInputImage, TOutputImage, TDistancePixel>
::Distance(const ClusterType &cluster, const InputPixelType &v, const PointType &pt)
{
  const unsigned int s = cluster.size();
  DistanceType d1 = 0.0;
  DistanceType d2 = 0.0;
  unsigned int i = 0;
  for (; i<s-ImageDimension; ++i)
    {
    const DistanceType d = (cluster[i] - v[i]);
    d1 += d*d;
    }
  //d1 = std::sqrt(d1);

  for (unsigned int j = 0; j < ImageDimension; ++j)
    {
    const DistanceType d = (cluster[i] - pt[j]) * m_DistanceScales[j];
    d2 += d*d;
    ++i;
    }
  //d2 = std::sqrt(d2);
  return d1+d2;
}

} // end namespace itk

#endif // itkSLICImageFilter_hxx