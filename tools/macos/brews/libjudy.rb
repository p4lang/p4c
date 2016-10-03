class Libjudy < Formula
  desc "Judy is a general purpose dynamic array implemented as a C callable library"
  homepage "https://sourceforge.net/projects/judy"
  url "https://sourceforge.net/projects/judy/files/judy/Judy-1.0.5/Judy-1.0.5.tar.gz"
  sha256 "d2704089f85fdb6f2cd7e77be21170ced4b4375c03ef1ad4cf1075bd414a63eb"

  depends_on "autoconf"

  def install
    ENV.j1
    system "./configure", "--prefix=#{prefix}"
    system "make", "check"
    system "make", "install"
  end

  test do
    system "make", "check"
  end
end
