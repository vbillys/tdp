#include <tdp/gl/render.h>

namespace tdp {

void RenderVboIds(
  pangolin::GlBuffer& vbo,
  const pangolin::OpenGlRenderState& cam
  ) {
  pangolin::GlSlProgram& shader = tdp::Shaders::Instance()->colorByIdShader_;
  shader.Bind();
  shader.SetUniform("P",cam.GetProjectionMatrix());
  shader.SetUniform("MV",cam.GetModelViewMatrix());
  vbo.Bind();
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0); 
  glEnableVertexAttribArray(0);                                               
  glDrawArrays(GL_POINTS, 0, vbo.num_elements);
  shader.Unbind();
  glDisableVertexAttribArray(0);
  vbo.Unbind();
}

void RenderLabeledVbo(
  pangolin::GlBuffer& vbo,
  pangolin::GlBuffer& labelbo,
  const pangolin::OpenGlRenderState& cam,
  int32_t labelOffset
    ) {
  pangolin::GlSlProgram& shader = tdp::Shaders::Instance()->labelShader_;
  shader.Bind();
  glEnable(GL_TEXTURE_2D);
  tdp::Labels::Instance()->Bind();
  shader.SetUniform("P",cam.GetProjectionMatrix());
  shader.SetUniform("MV",cam.GetModelViewMatrix());
  shader.SetUniform("labels",0);
  shader.SetUniform("offset",(float)labelOffset);
  labelbo.Bind();
  glVertexAttribPointer(1, 1, GL_UNSIGNED_SHORT, GL_FALSE, 0, 0); 
  vbo.Bind();
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0); 
  glEnableVertexAttribArray(0);                                               
  glEnableVertexAttribArray(1);                                               
  //pangolin::RenderVbo(nboA);
  glDrawArrays(GL_POINTS, 0, vbo.num_elements);
  tdp::Labels::Instance()->Unbind();
  glDisable(GL_TEXTURE_2D);
  shader.Unbind();
  glDisableVertexAttribArray(1);
  labelbo.Unbind();
  glDisableVertexAttribArray(0);
  vbo.Unbind();
}

void RenderVboIbo(
  pangolin::GlBuffer& vbo,
  pangolin::GlBuffer& ibo
    ) {
	vbo.Bind();
	glVertexPointer(vbo.count_per_element, vbo.datatype, 0, 0);
	glEnableClientState(GL_VERTEX_ARRAY);

	ibo.Bind();
	glDrawElements(GL_TRIANGLES,ibo.num_elements*3, ibo.datatype, 0);
	ibo.Unbind();

	glDisableClientState(GL_VERTEX_ARRAY);
	vbo.Unbind();
}

void RenderVboIboCbo(
  pangolin::GlBuffer& vbo,
  pangolin::GlBuffer& ibo,
  pangolin::GlBuffer& cbo
    ) {
	cbo.Bind();
	glColorPointer(cbo.count_per_element, cbo.datatype, 0, 0);
	glEnableClientState(GL_COLOR_ARRAY);

	tdp::RenderVboIbo(vbo,ibo);

	glDisableClientState(GL_COLOR_ARRAY);
	cbo.Unbind();
}

void RenderSurfels(
  const pangolin::GlBuffer& vbo,
  const pangolin::GlBuffer& nbo,
  const pangolin::GlBuffer& cbo,
  const pangolin::GlBuffer& rbo,
  const pangolin::OpenGlMatrix& MVP
    ) {

  pangolin::GlSlProgram& shader = tdp::Shaders::Instance()->surfelShader_;  
  shader.Bind();
  shader.SetUniform("MVP",MVP);

  vbo.Bind();
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0); 
  cbo.Bind();
  glVertexAttribPointer(1, 3, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0); 
  nbo.Bind();
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0); 
  rbo.Bind();
  glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 0, 0); 

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);
  glEnableVertexAttribArray(3);

  glDrawArrays(GL_POINTS, 0, vbo.num_elements);

  glDisableVertexAttribArray(3);
  rbo.Unbind();
  glDisableVertexAttribArray(2);
  nbo.Unbind();
  glDisableVertexAttribArray(1);
  cbo.Unbind();
  glDisableVertexAttribArray(0);
  vbo.Unbind();
  shader.Unbind();
}

void RenderSurfelsValue(
    const pangolin::GlBuffer& vbo,
    const pangolin::GlBuffer& nbo,
    const pangolin::GlBuffer& rbo,
    const pangolin::GlBuffer& valuebo,
    float minVal, float maxVal,
    const pangolin::OpenGlMatrix& MVP
    ) {
  pangolin::GlSlProgram& shader = Shaders::Instance()->surfelValueShader_;

  shader.Bind();
  shader.SetUniform("MVP",MVP);
  shader.SetUniform("minValue", minVal);
  shader.SetUniform("maxValue", maxVal);

  vbo.Bind();
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0); 
  valuebo.Bind();
  glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, 0); 
  nbo.Bind();
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0); 
  rbo.Bind();
  glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 0, 0); 

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);
  glEnableVertexAttribArray(3);

  glDrawArrays(GL_POINTS, 0, vbo.num_elements);

  glDisableVertexAttribArray(3);
  rbo.Unbind();
  glDisableVertexAttribArray(2);
  nbo.Unbind();
  glDisableVertexAttribArray(1);
  valuebo.Unbind();
  glDisableVertexAttribArray(0);
  vbo.Unbind();
  shader.Unbind();

}

void RenderSurfelsLabeled(
    const pangolin::GlBuffer& vbo,
    const pangolin::GlBuffer& nbo,
    const pangolin::GlBuffer& rbo,
    const pangolin::GlBuffer& labelbo,
    int32_t labelOffset,
    const pangolin::OpenGlMatrix& MVP
    ) {
  pangolin::GlSlProgram& shader = tdp::Shaders::Instance()->surfelLabelShader_;
  shader.Bind();
  glEnable(GL_TEXTURE_2D);
  tdp::Labels::Instance()->Bind();
  shader.SetUniform("MVP",MVP);
  shader.SetUniform("labels",0);
  shader.SetUniform("offset",(float)labelOffset);

  vbo.Bind();
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0); 
  labelbo.Bind();
  glVertexAttribPointer(1, 1, GL_UNSIGNED_SHORT, GL_FALSE, 0, 0); 
  nbo.Bind();
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0); 
  rbo.Bind();
  glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 0, 0); 

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);
  glEnableVertexAttribArray(3);

  glDrawArrays(GL_POINTS, 0, vbo.num_elements);

  tdp::Labels::Instance()->Unbind();
  glDisable(GL_TEXTURE_2D);
  glDisableVertexAttribArray(3);
  rbo.Unbind();
  glDisableVertexAttribArray(2);
  nbo.Unbind();
  glDisableVertexAttribArray(1);
  labelbo.Unbind();
  glDisableVertexAttribArray(0);
  vbo.Unbind();
  shader.Unbind();
}

void RenderVboValuebo(
    const pangolin::GlBuffer& vbo,
    const pangolin::GlBuffer& valuebo,
    float minVal, float maxVal,
    const pangolin::OpenGlMatrix& P,
    const pangolin::OpenGlMatrix& MV
    ) {
  pangolin::GlSlProgram& shader = Shaders::Instance()->valueShader_;

  shader.Bind();
  shader.SetUniform("P", P);
  shader.SetUniform("MV",MV);
  shader.SetUniform("minValue", minVal);
  shader.SetUniform("maxValue", maxVal);

  vbo.Bind();
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0); 
  valuebo.Bind();
  glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, 0); 

  glEnableVertexAttribArray(0);                                               
  glEnableVertexAttribArray(1);                                               

  glDrawArrays(GL_POINTS, 0, vbo.num_elements);

  shader.Unbind();
  glDisableVertexAttribArray(1);
  valuebo.Unbind();
  glDisableVertexAttribArray(0);
  vbo.Unbind();

}

}
