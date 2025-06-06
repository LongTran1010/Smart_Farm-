from tensorflow.keras.layers import DepthwiseConv2D as _TFDepthwiseConv2D

class DepthwiseConv2D(_TFDepthwiseConv2D):
    def __init__(self, *args, **kwargs):
        # pop 'groups' nếu có
        kwargs.pop('groups', None)
        super().__init__(*args, **kwargs)