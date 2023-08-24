QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets multimedia concurrent

CONFIG += c++14

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    Apps/AppNetworkProcess/arcface_process.cpp \
    Apps/AppNetworkProcess/lprnet_process.cpp \
    Apps/AppNetworkProcess/yolov4Tiny_license_plate_det_process.cpp \
    Apps/AppNetworkProcess/yolov5_faceDet_process.cpp \
    Apps/AppNetworkProcess/yolov5_instance_seg_process.cpp \
    Apps/AppNetworkProcess/yolov5_process.cpp \
    Apps/AppNetworkProcess/yolov5_vehicle_process.cpp \
    Apps/AppNetworkProcess/yolov7_process.cpp \
    Apps/AppNetworkProcess/yolov7tiny_process.cpp \
    Apps/FaceRecognition/faceRecognition.cpp \
    Apps/LPR/Lpr.cpp \
    Apps/ObjectDetection/objectdetection.cpp \
    Apps/Segmentation/segmentation.cpp \
    Apps/appbaseclass.cpp \
    Apps/appmanager.cpp \
    Utils/ctc_beam_search_decoder-master/src/ctc_beam_search_decoder.cpp \
    Utils/database/FaceDatabase.cpp \
    Utils/tracking/hailo_tracker.cpp \
    main.cpp \
    mainwindow.cpp \
    streamcontainer.cpp \
    streamview.cpp \
    videoview.cpp \

HEADERS += \
    Apps/AppNetworkProcess/arcface_process.h \
    Apps/AppNetworkProcess/lprnet_process.h \
    Apps/AppNetworkProcess/yolov4Tiny_license_plate_det_process.h \
    Apps/AppNetworkProcess/yolov5_faceDet_process.h \
    Apps/AppNetworkProcess/yolov5_instance_seg_process.h \
    Apps/AppNetworkProcess/yolov5_process.h \
    Apps/AppNetworkProcess/yolov5_vehicle_process.h \
    Apps/AppNetworkProcess/yolov7_process.h \
    Apps/AppNetworkProcess/yolov7tiny_process.h \
    Apps/AppsFactory.h \
    Apps/FaceRecognition/faceRecognition.h \
    Apps/LPR/Lpr.h \
    Apps/ObjectDetection/objectdetection.h \
    Apps/Segmentation/segmentation.h \
    Apps/appbaseclass.h \
    Apps/appcommon.h \
    Apps/appmanager.h \
    #Apps/appsequencer.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/Cholesky \
    Utils/ctc_beam_search_decoder-master/src/Eigen/CholmodSupport \
    Utils/ctc_beam_search_decoder-master/src/Eigen/Core \
    Utils/ctc_beam_search_decoder-master/src/Eigen/Dense \
    Utils/ctc_beam_search_decoder-master/src/Eigen/Eigen \
    Utils/ctc_beam_search_decoder-master/src/Eigen/Eigenvalues \
    Utils/ctc_beam_search_decoder-master/src/Eigen/Geometry \
    Utils/ctc_beam_search_decoder-master/src/Eigen/Householder \
    Utils/ctc_beam_search_decoder-master/src/Eigen/IterativeLinearSolvers \
    Utils/ctc_beam_search_decoder-master/src/Eigen/Jacobi \
    Utils/ctc_beam_search_decoder-master/src/Eigen/KLUSupport \
    Utils/ctc_beam_search_decoder-master/src/Eigen/LU \
    Utils/ctc_beam_search_decoder-master/src/Eigen/MetisSupport \
    Utils/ctc_beam_search_decoder-master/src/Eigen/OrderingMethods \
    Utils/ctc_beam_search_decoder-master/src/Eigen/PaStiXSupport \
    Utils/ctc_beam_search_decoder-master/src/Eigen/PardisoSupport \
    Utils/ctc_beam_search_decoder-master/src/Eigen/QR \
    Utils/ctc_beam_search_decoder-master/src/Eigen/QtAlignedMalloc \
    Utils/ctc_beam_search_decoder-master/src/Eigen/SPQRSupport \
    Utils/ctc_beam_search_decoder-master/src/Eigen/SVD \
    Utils/ctc_beam_search_decoder-master/src/Eigen/Sparse \
    Utils/ctc_beam_search_decoder-master/src/Eigen/SparseCholesky \
    Utils/ctc_beam_search_decoder-master/src/Eigen/SparseCore \
    Utils/ctc_beam_search_decoder-master/src/Eigen/SparseLU \
    Utils/ctc_beam_search_decoder-master/src/Eigen/SparseQR \
    Utils/ctc_beam_search_decoder-master/src/Eigen/StdDeque \
    Utils/ctc_beam_search_decoder-master/src/Eigen/StdList \
    Utils/ctc_beam_search_decoder-master/src/Eigen/StdVector \
    Utils/ctc_beam_search_decoder-master/src/Eigen/SuperLUSupport \
    Utils/ctc_beam_search_decoder-master/src/Eigen/UmfPackSupport \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Cholesky/LDLT.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Cholesky/LLT.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Cholesky/LLT_LAPACKE.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/CholmodSupport/CholmodSupport.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/ArithmeticSequence.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/Array.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/ArrayBase.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/ArrayWrapper.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/Assign.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/AssignEvaluator.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/Assign_MKL.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/BandMatrix.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/Block.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/BooleanRedux.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/CommaInitializer.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/ConditionEstimator.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/CoreEvaluators.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/CoreIterators.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/CwiseBinaryOp.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/CwiseNullaryOp.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/CwiseTernaryOp.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/CwiseUnaryOp.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/CwiseUnaryView.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/DenseBase.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/DenseCoeffsBase.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/DenseStorage.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/Diagonal.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/DiagonalMatrix.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/DiagonalProduct.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/Dot.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/EigenBase.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/ForceAlignedAccess.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/Fuzzy.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/GeneralProduct.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/GenericPacketMath.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/GlobalFunctions.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/IO.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/IndexedView.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/Inverse.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/Map.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/MapBase.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/MathFunctions.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/MathFunctionsImpl.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/Matrix.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/MatrixBase.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/NestByValue.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/NoAlias.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/NumTraits.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/PartialReduxEvaluator.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/PermutationMatrix.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/PlainObjectBase.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/Product.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/ProductEvaluators.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/Random.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/Redux.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/Ref.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/Replicate.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/Reshaped.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/ReturnByValue.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/Reverse.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/Select.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/SelfAdjointView.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/SelfCwiseBinaryOp.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/Solve.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/SolveTriangular.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/SolverBase.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/StableNorm.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/StlIterators.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/Stride.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/Swap.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/Transpose.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/Transpositions.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/TriangularMatrix.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/VectorBlock.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/VectorwiseOp.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/Visitor.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/arch/AVX/Complex.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/arch/AVX/MathFunctions.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/arch/AVX/PacketMath.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/arch/AVX/TypeCasting.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/arch/AVX512/Complex.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/arch/AVX512/MathFunctions.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/arch/AVX512/PacketMath.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/arch/AVX512/TypeCasting.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/arch/AltiVec/Complex.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/arch/AltiVec/MathFunctions.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/arch/AltiVec/PacketMath.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/arch/CUDA/Complex.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/arch/Default/ConjHelper.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/arch/Default/GenericPacketMathFunctions.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/arch/Default/GenericPacketMathFunctionsFwd.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/arch/Default/Half.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/arch/Default/Settings.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/arch/Default/TypeCasting.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/arch/GPU/MathFunctions.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/arch/GPU/PacketMath.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/arch/GPU/TypeCasting.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/arch/HIP/hcc/math_constants.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/arch/MSA/Complex.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/arch/MSA/MathFunctions.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/arch/MSA/PacketMath.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/arch/NEON/Complex.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/arch/NEON/MathFunctions.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/arch/NEON/PacketMath.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/arch/NEON/TypeCasting.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/arch/SSE/Complex.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/arch/SSE/MathFunctions.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/arch/SSE/PacketMath.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/arch/SSE/TypeCasting.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/arch/SYCL/InteropHeaders.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/arch/SYCL/MathFunctions.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/arch/SYCL/PacketMath.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/arch/SYCL/SyclMemoryModel.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/arch/SYCL/TypeCasting.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/arch/ZVector/Complex.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/arch/ZVector/MathFunctions.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/arch/ZVector/PacketMath.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/functors/AssignmentFunctors.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/functors/BinaryFunctors.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/functors/NullaryFunctors.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/functors/StlFunctors.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/functors/TernaryFunctors.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/functors/UnaryFunctors.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/products/GeneralBlockPanelKernel.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/products/GeneralMatrixMatrix.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/products/GeneralMatrixMatrixTriangular.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/products/GeneralMatrixMatrixTriangular_BLAS.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/products/GeneralMatrixMatrix_BLAS.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/products/GeneralMatrixVector.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/products/GeneralMatrixVector_BLAS.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/products/Parallelizer.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/products/SelfadjointMatrixMatrix.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/products/SelfadjointMatrixMatrix_BLAS.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/products/SelfadjointMatrixVector.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/products/SelfadjointMatrixVector_BLAS.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/products/SelfadjointProduct.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/products/SelfadjointRank2Update.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/products/TriangularMatrixMatrix.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/products/TriangularMatrixMatrix_BLAS.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/products/TriangularMatrixVector.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/products/TriangularMatrixVector_BLAS.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/products/TriangularSolverMatrix.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/products/TriangularSolverMatrix_BLAS.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/products/TriangularSolverVector.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/util/BlasUtil.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/util/ConfigureVectorization.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/util/Constants.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/util/DisableStupidWarnings.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/util/ForwardDeclarations.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/util/IndexedViewHelper.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/util/IntegralConstant.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/util/MKL_support.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/util/Macros.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/util/Memory.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/util/Meta.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/util/NonMPL2.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/util/ReenableStupidWarnings.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/util/ReshapedHelper.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/util/StaticAssert.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/util/SymbolicIndex.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Core/util/XprHelper.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Eigenvalues/ComplexEigenSolver.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Eigenvalues/ComplexSchur.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Eigenvalues/ComplexSchur_LAPACKE.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Eigenvalues/EigenSolver.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Eigenvalues/GeneralizedEigenSolver.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Eigenvalues/GeneralizedSelfAdjointEigenSolver.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Eigenvalues/HessenbergDecomposition.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Eigenvalues/MatrixBaseEigenvalues.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Eigenvalues/RealQZ.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Eigenvalues/RealSchur.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Eigenvalues/RealSchur_LAPACKE.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Eigenvalues/SelfAdjointEigenSolver.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Eigenvalues/SelfAdjointEigenSolver_LAPACKE.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Eigenvalues/Tridiagonalization.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Geometry/AlignedBox.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Geometry/AngleAxis.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Geometry/EulerAngles.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Geometry/Homogeneous.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Geometry/Hyperplane.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Geometry/OrthoMethods.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Geometry/ParametrizedLine.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Geometry/Quaternion.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Geometry/Rotation2D.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Geometry/RotationBase.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Geometry/Scaling.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Geometry/Transform.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Geometry/Translation.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Geometry/Umeyama.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Geometry/arch/Geometry_SSE.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Householder/BlockHouseholder.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Householder/Householder.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Householder/HouseholderSequence.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/IterativeLinearSolvers/BasicPreconditioners.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/IterativeLinearSolvers/BiCGSTAB.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/IterativeLinearSolvers/ConjugateGradient.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/IterativeLinearSolvers/IncompleteCholesky.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/IterativeLinearSolvers/IncompleteLUT.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/IterativeLinearSolvers/IterativeSolverBase.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/IterativeLinearSolvers/LeastSquareConjugateGradient.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/IterativeLinearSolvers/SolveWithGuess.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/Jacobi/Jacobi.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/KLUSupport/KLUSupport.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/LU/Determinant.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/LU/FullPivLU.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/LU/InverseImpl.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/LU/PartialPivLU.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/LU/PartialPivLU_LAPACKE.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/LU/arch/Inverse_SSE.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/MetisSupport/MetisSupport.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/OrderingMethods/Amd.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/OrderingMethods/Eigen_Colamd.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/OrderingMethods/Ordering.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/PaStiXSupport/PaStiXSupport.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/PardisoSupport/PardisoSupport.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/QR/ColPivHouseholderQR.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/QR/ColPivHouseholderQR_LAPACKE.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/QR/CompleteOrthogonalDecomposition.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/QR/FullPivHouseholderQR.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/QR/HouseholderQR.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/QR/HouseholderQR_LAPACKE.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SPQRSupport/SuiteSparseQRSupport.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SVD/BDCSVD.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SVD/JacobiSVD.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SVD/JacobiSVD_LAPACKE.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SVD/SVDBase.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SVD/UpperBidiagonalization.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseCholesky/SimplicialCholesky.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseCholesky/SimplicialCholesky_impl.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseCore/AmbiVector.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseCore/CompressedStorage.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseCore/ConservativeSparseSparseProduct.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseCore/MappedSparseMatrix.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseCore/SparseAssign.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseCore/SparseBlock.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseCore/SparseColEtree.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseCore/SparseCompressedBase.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseCore/SparseCwiseBinaryOp.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseCore/SparseCwiseUnaryOp.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseCore/SparseDenseProduct.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseCore/SparseDiagonalProduct.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseCore/SparseDot.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseCore/SparseFuzzy.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseCore/SparseMap.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseCore/SparseMatrix.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseCore/SparseMatrixBase.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseCore/SparsePermutation.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseCore/SparseProduct.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseCore/SparseRedux.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseCore/SparseRef.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseCore/SparseSelfAdjointView.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseCore/SparseSolverBase.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseCore/SparseSparseProductWithPruning.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseCore/SparseTranspose.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseCore/SparseTriangularView.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseCore/SparseUtil.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseCore/SparseVector.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseCore/SparseView.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseCore/TriangularSolver.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseLU/SparseLU.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseLU/SparseLUImpl.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseLU/SparseLU_Memory.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseLU/SparseLU_Structs.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseLU/SparseLU_SupernodalMatrix.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseLU/SparseLU_Utils.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseLU/SparseLU_column_bmod.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseLU/SparseLU_column_dfs.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseLU/SparseLU_copy_to_ucol.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseLU/SparseLU_gemm_kernel.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseLU/SparseLU_heap_relax_snode.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseLU/SparseLU_kernel_bmod.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseLU/SparseLU_panel_bmod.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseLU/SparseLU_panel_dfs.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseLU/SparseLU_pivotL.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseLU/SparseLU_pruneL.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseLU/SparseLU_relax_snode.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SparseQR/SparseQR.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/StlSupport/StdDeque.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/StlSupport/StdList.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/StlSupport/StdVector.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/StlSupport/details.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/SuperLUSupport/SuperLUSupport.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/UmfPackSupport/UmfPackSupport.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/misc/Image.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/misc/Kernel.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/misc/RealSvd2x2.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/misc/blas.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/misc/lapack.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/misc/lapacke.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/misc/lapacke_mangling.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/plugins/ArrayCwiseBinaryOps.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/plugins/ArrayCwiseUnaryOps.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/plugins/BlockMethods.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/plugins/CommonCwiseBinaryOps.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/plugins/CommonCwiseUnaryOps.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/plugins/IndexedViewMethods.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/plugins/MatrixCwiseBinaryOps.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/plugins/MatrixCwiseUnaryOps.h \
    Utils/ctc_beam_search_decoder-master/src/Eigen/src/plugins/ReshapedMethods.h \
    Utils/ctc_beam_search_decoder-master/src/ctc_beam_entry.h \
    Utils/ctc_beam_search_decoder-master/src/ctc_beam_scorer.h \
    Utils/ctc_beam_search_decoder-master/src/ctc_beam_search.h \
    Utils/ctc_beam_search_decoder-master/src/ctc_beam_search_decoder.h \
    Utils/ctc_beam_search_decoder-master/src/ctc_decoder.h \
    Utils/ctc_beam_search_decoder-master/src/ctc_math.h \
    Utils/ctc_beam_search_decoder-master/src/macros.h \
    Utils/ctc_beam_search_decoder-master/src/top_n.h \
    Utils/database/FaceDatabase.hpp \
    Utils/hailo-common/hailo_common.hpp \
    Utils/hailo-common/hailo_objects.hpp \
    Utils/hailo-common/hailo_tensors.hpp \
    Utils/tracking/hailo_tracker.hpp \
    Utils/tracking/jde_tracker/jde_tracker.hpp \
    Utils/tracking/jde_tracker/jde_tracker_converters.hpp \
    Utils/tracking/jde_tracker/jde_tracker_embedding.hpp \
    Utils/tracking/jde_tracker/jde_tracker_ious.hpp \
    Utils/tracking/jde_tracker/jde_tracker_lapjv.hpp \
    Utils/tracking/jde_tracker/jde_tracker_strack_management.hpp \
    Utils/tracking/jde_tracker/jde_tracker_update.hpp \
    Utils/tracking/jde_tracker/kalman_filter.hpp \
    Utils/tracking/jde_tracker/lapjv.hpp \
    Utils/tracking/jde_tracker/strack.hpp \
    Utils/tracking/jde_tracker/tracker_macros.hpp \
    Utils/yolo-nms-decoder/yolo_nms_decoder.hpp \
    mainwindow.h \
    streamcontainer.h \
    streamview.h \
    videoview.h \

lessThan(QT_MAJOR_VERSION, 6): SOURCES +=   mediastream.cpp
lessThan(QT_MAJOR_VERSION, 6): HEADERS +=   mediastream.h
greaterThan(QT_MAJOR_VERSION, 5): SOURCES +=    mediastreamQt6.cpp
greaterThan(QT_MAJOR_VERSION, 5): HEADERS +=    mediastreamQt6.h

lessThan(QT_MAJOR_VERSION, 6): SOURCES +=   cameraview.cpp
lessThan(QT_MAJOR_VERSION, 6): HEADERS +=   cameraview.h
greaterThan(QT_MAJOR_VERSION, 5): SOURCES += cameraviewQt6.cpp
greaterThan(QT_MAJOR_VERSION, 5): HEADERS += cameraviewQt6.h

INCLUDEPATH += $$PWD/Utils/hailo-common
INCLUDEPATH += $$PWD/Utils/tracking
INCLUDEPATH += $$PWD/Utils/xtensor-master/include
INCLUDEPATH += $$PWD/Utils/xtl-master/include

FORMS += \
    mainwindow.ui

# Project Defines (Enable one only)
DEFINES += QT_ON_x86
# DEFINES += QT_ON_JETSON
# DEFINES += QT_ON_RK3588

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /home/jetsoon/Desktop
!isEmpty(target.path): INSTALLS += target

# Linux MultiNetworkPipeline
unix:!macx: LIBS += -L$$PWD/MultiNetworkPipeline-Scheduler/ -lMultiNetworkPipeline
INCLUDEPATH += $$PWD/MultiNetworkPipeline-Scheduler
DEPENDPATH += $$PWD/MultiNetworkPipeline-Scheduler
unix:!macx: PRE_TARGETDEPS += $$PWD/MultiNetworkPipeline-Scheduler/libMultiNetworkPipeline.a

# Windows MultiNetworkPipeline
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/MultiNetworkPipeline-Scheduler/release/ -lMultiNetworkPipeline
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/MultiNetworkPipeline-Scheduler/debug/ -lMultiNetworkPipeline
INCLUDEPATH += $$PWD/MultiNetworkPipeline-Scheduler
DEPENDPATH += $$PWD/MultiNetworkPipeline-Scheduler

# Linux HailoRT
unix:!macx: LIBS += -L$$PWD/../../../../../usr/lib/ -lhailort
INCLUDEPATH += $$PWD/../../../../../usr/include/hailo
DEPENDPATH += $$PWD/../../../../../usr/include/hailo

# WINDOWS HailoRT
win32: LIBS += -L$$PWD/'../../../../Program Files/HailoRT/lib/' -llibhailort
INCLUDEPATH += $$PWD/'../../../../Program Files/HailoRT/include'
DEPENDPATH += $$PWD/'../../../../Program Files/HailoRT/include'

# Windows OpenCV
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../OpenCV-4.5.4/opencv/build/x64/vc15/lib/ -lopencv_world454
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../OpenCV-4.5.4/opencv/build/x64/vc15/lib/ -lopencv_world454d

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../OpenCV-4.5.4/opencv/build/x64/vc15/bin/ -lopencv_world454
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../OpenCV-4.5.4/opencv/build/x64/vc15/bin/ -lopencv_world454d

INCLUDEPATH += $$PWD/../../../../OpenCV-4.5.4/opencv/build/include
DEPENDPATH += $$PWD/../../../../OpenCV-4.5.4/opencv/build/include

unix:CONFIG += link_pkgconfig
unix:PKGCONFIG += opencv4
