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
#ifndef itkObjectnessMeasureImageFilter_h
#define itkObjectnessMeasureImageFilter_h


#include "itkHessianRecursiveGaussianImageFilter.h"
#include "itkHessianToObjectnessMeasureImageFilter.h"
#include "itkProgressAccumulator.h"


namespace itk
{
/** \class ObjectnessMeasureImageFilter
 *
 * This is a composite filter which combine a computation of the
 * hessian with computation of the objectness.
 *
 * \ingroup SimpleITKFiltersModule
 */
template< typename TInputImage, typename TOutputImage >
class ObjectnessMeasureImageFilter
  : public ImageToImageFilter< TInputImage, TOutputImage >
{
public:
  /** Standard class typedefs. */
  typedef ObjectnessMeasureImageFilter Self;

  typedef ImageToImageFilter< TInputImage, TOutputImage > Superclass;

  typedef SmartPointer< Self >       Pointer;
  typedef SmartPointer< const Self > ConstPointer;

  typedef typename Superclass::InputImageType  InputImageType;
  typedef typename Superclass::OutputImageType OutputImageType;

  typedef double                                   InternalType;

  /** Image dimension */
  itkStaticConstMacro(ImageDimension, unsigned int,  InputImageType ::ImageDimension);


  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Runtime information support. */
  itkTypeMacro(ObjectnessMeasureImageFilter, ImageToImageFilter);


  /** Set/Get Alpha, the weight corresponding to R_A
   * (the ratio of the smallest eigenvalue that has to be large to the larger ones).
   * Smaller values lead to increased sensitivity to the object dimensionality. */
  itkSetMacro(Alpha, double);
  itkGetConstMacro(Alpha, double);

  /** Set/Get Beta, the weight corresponding to R_B
   * (the ratio of the largest eigenvalue that has to be small to the larger ones).
   * Smaller values lead to increased sensitivity to the object dimensionality. */
  itkSetMacro(Beta, double);
  itkGetConstMacro(Beta, double);

  /** Set/Get Gamma, the weight corresponding to S
   * (the Frobenius norm of the Hessian matrix, or second-order structureness) */
  itkSetMacro(Gamma, double);
  itkGetConstMacro(Gamma, double);

  /** Toggle scaling the objectness measure with the magnitude of the largest
    absolute eigenvalue */
  itkSetMacro(ScaleObjectnessMeasure, bool);
  itkGetConstMacro(ScaleObjectnessMeasure, bool);
  itkBooleanMacro(ScaleObjectnessMeasure);

  /** Set/Get the dimensionality of the object (0: points (blobs),
   * 1: lines (vessels), 2: planes (plate-like structures), 3: hyper-planes.
   * ObjectDimension must be smaller than ImageDimension. */
  itkSetMacro(ObjectDimension, unsigned int);
  itkGetConstMacro(ObjectDimension, unsigned int);

  /** Enhance bright structures on a dark background if true, the opposite if
    false. */
  itkSetMacro(BrightObject, bool);
  itkGetConstMacro(BrightObject, bool);
  itkBooleanMacro(BrightObject);


protected:
  ObjectnessMeasureImageFilter()
    :  m_Alpha( 0.5 ),
       m_Beta( 0.5 ),
       m_Gamma( 5.0 ),
       m_ObjectDimension( 1 ),
       m_BrightObject( true ),
       m_ScaleObjectnessMeasure( true )
    { }

  ~ObjectnessMeasureImageFilter() {}


  void EnlargeOutputRequestedRegion(DataObject *output)
    {
      output->SetRequestedRegionToLargestPossibleRegion();
    }

  virtual void GenerateData() ITK_OVERRIDE
    {
      // Create a process accumulator for tracking the progress of this minipipeline
      ProgressAccumulator::Pointer progress = ProgressAccumulator::New();
      progress->SetMiniPipelineFilter( this );

      typename InputImageType::Pointer localInput = InputImageType::New();
      localInput->Graft( this->GetInput() );

      typedef HessianRecursiveGaussianImageFilter<InputImageType> HessianFilterType;
      typedef typename HessianFilterType::OutputImageType         HessianImageType;

      typename HessianFilterType::Pointer hessianFilter = HessianFilterType::New();

      hessianFilter->SetInput( localInput );

      hessianFilter->SetSigma( 1.0 );


      typedef HessianToObjectnessMeasureImageFilter< HessianImageType, OutputImageType > ObjectnessFilterType;

      typename ObjectnessFilterType::Pointer objectnessFilter = ObjectnessFilterType::New();

      objectnessFilter->SetInput( hessianFilter->GetOutput() );

      objectnessFilter->SetAlpha( m_Alpha );
      objectnessFilter->SetBeta( m_Beta );
      objectnessFilter->SetGamma( m_Gamma );
      objectnessFilter->SetScaleObjectnessMeasure( m_ScaleObjectnessMeasure );
      objectnessFilter->SetObjectDimension( m_ObjectDimension );
      objectnessFilter->SetBrightObject( m_BrightObject);

      progress->RegisterInternalFilter( hessianFilter, 0.5f );
      progress->RegisterInternalFilter( objectnessFilter, 0.5f );


      objectnessFilter->GraftOutput( this->GetOutput() );
      objectnessFilter->Update();
      this->GraftOutput(objectnessFilter->GetOutput());
    }


private:
  ITK_DISALLOW_COPY_AND_ASSIGN(ObjectnessMeasureImageFilter);

  double       m_Alpha;
  double       m_Beta;
  double       m_Gamma;
  unsigned int m_ObjectDimension;
  bool         m_BrightObject;
  bool         m_ScaleObjectnessMeasure;

};

}

#endif // itkObjectnessMeasureImageFilter_h
