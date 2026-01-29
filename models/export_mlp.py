import torch, torch.nn as nn

class TinyMLP(nn.Module):
    def __init__(self, in_dim=4, hidden=8, out_dim=3):
        super().__init__()
        self.net = nn.Sequential(
            nn.Linear(in_dim, hidden), nn.ReLU(),
            nn.Linear(hidden, out_dim)
        )
    def forward(self, x): return self.net(x)

torch.manual_seed(0)
m = TinyMLP()
m.eval()

x = torch.randn(1, 4)  # single sample
torch.onnx.export(
    m, x, "models/mlp.onnx",
    input_names=["input"], output_names=["logits"],
    dynamic_axes={"input": {0: "N"}, "logits": {0: "N"}},
    opset_version=17
)
print("wrote models/mlp.onnx")
