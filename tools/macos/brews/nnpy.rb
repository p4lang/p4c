class Nnpy < Formula
  desc "cffi-based Python bindings for nanomsg"
  homepage "https://github.com/nanomsg/nnpy"
  url "https://github.com/nanomsg/nnpy.git", :revision => "c7e718a5173447c85182dc45f99e2abcf9cd4065"
  version "1.2"

  depends_on :python
  depends_on "nanomsg"
  depends_on "pkg-config"
  depends_on "libffi"

  resource "cffi" do
    url "https://pypi.python.org/packages/0a/f3/686af8873b70028fccf67b15c78fd4e4667a3da995007afc71e786d61b0a/cffi-1.8.3.tar.gz#md5=c8e877fe0426a99d0cf5872cf2f95b27"
    sha256 "c321bd46faa7847261b89c0469569530cad5a41976bb6dba8202c0159f476568"
  end

  def install
    version = Language::Python.major_minor_version "python"
    dest_path = lib/"python#{version}/site-packages"
    dest_path.mkpath

    bundle_path = libexec/"lib/python#{version}/site-packages"
    ENV.prepend_create_path "PYTHONPATH", bundle_path
    resource("cffi").stage do
      system "python", *Language::Python.setup_install_args(libexec)
    end
    (dest_path/"homebrew-nnpy-bundle.pth").write "#{bundle_path}\n"

    ENV.prepend_create_path "PYTHONPATH", prefix/"lib/python#{version}/site-packages"
    system "python", *Language::Python.setup_install_args(prefix)
  end

  test do
    system "true"
  end
end
