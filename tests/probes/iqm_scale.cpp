// IQM joints apply local scale before rotation, followed by translation.
#include "../../engine/render/tr_model_iqm.cpp"
#include <assert.h>

int main() {
	const vec3_t scales[] = { { 2, 3, 4 }, { -2, 3, 0.5f }, { 1, 1, 1 } };
	const vec3_t translation = { 5, 6, 7 }, vertex = { 1, 2, 3 };
	for ( const auto &scale : scales ) {
		for ( int axis = 0; axis < 3; ++axis ) {
			quat_t rotation = { 0, 0, 0, 0.7071067811865475f };
			rotation[axis] = rotation[3];
			float matrix[12], inverse[12], product[12];
			JointToMatrix( rotation, scale, translation, matrix );
			const vec3_t scaled = { scale[0] * vertex[0], scale[1] * vertex[1], scale[2] * vertex[2] };
			const vec3_t expected[] = {
				{ scaled[0], -scaled[2], scaled[1] },
				{ scaled[2], scaled[1], -scaled[0] },
				{ -scaled[1], scaled[0], scaled[2] }
			};
			for ( int row = 0; row < 3; ++row ) {
				const float actual = DotProduct( matrix + row * 4, vertex ) + matrix[row * 4 + 3];
				assert( fabsf( actual - expected[axis][row] - translation[row] ) < 0.00001f );
			}
			Matrix34Invert( matrix, inverse );
			Matrix34Multiply( matrix, inverse, product );
			for ( int component = 0; component < 12; ++component )
				assert( fabsf( product[component] - identityMatrix[component] ) < 0.00001f );
		}
	}
	puts( "PASS: IQM joint scale precedes rotation on all axes; inverse round trips" );
}
